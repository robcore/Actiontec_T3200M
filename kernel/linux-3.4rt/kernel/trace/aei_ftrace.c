/*
 * Lin Ming <mlin@actionte.com>
 * Function trace into memory
 */

#include <linux/kernel.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/kallsyms.h>
#include <linux/bootmem.h>
#include <linux/proc_fs.h>
#include "trace.h"

#define MAGIC 0x41454966	/* "AEIf" */

static char *trace_buf;

struct aei_ftrace_record {
	unsigned long ip;
	unsigned long parent_ip;
};

#define PTRACE_IRQ_DISABLED 0x1
#define HARDIRQ 0x1
#define SOFTIRQ 0x2

#define REC_SIZE sizeof(struct aei_ftrace_record)
static int cur_index[2] = {REC_SIZE, REC_SIZE};

static void aei_ftrace_write(struct aei_ftrace_record *rec, unsigned long flags)
{
	struct aei_ftrace_record *r;
	int irq_flag = 0;
	int cpu = raw_smp_processor_id();
	char *buf = trace_buf + cpu * PAGE_SIZE;

	if (cur_index[cpu] >= PAGE_SIZE)
		cur_index[cpu] = REC_SIZE;

	if (in_irq())
		irq_flag |= HARDIRQ;
	if (in_softirq())
		irq_flag |= SOFTIRQ;

	r = (struct aei_ftrace_record *)(buf + cur_index[cpu]);
	r->ip = rec->ip;
	if (!(flags & 1))
		r->ip |= PTRACE_IRQ_DISABLED;
	r->parent_ip = rec->parent_ip | irq_flag;

	r = (struct aei_ftrace_record *)buf;
	r->parent_ip = cur_index[cpu] / REC_SIZE; /* The index of last trace */

	cur_index[cpu] += REC_SIZE;
}

static void aei_ftrace_call(unsigned long ip, unsigned long parent_ip)
{
	struct aei_ftrace_record rec;
	unsigned long flags;

	if (unlikely(oops_in_progress))
		return;

	local_irq_save(flags);
	rec.ip = ip;
	rec.parent_ip = parent_ip;
	aei_ftrace_write(&rec, flags);
	local_irq_restore(flags);
}

static struct ftrace_ops trace_ops __read_mostly = {
	.func = aei_ftrace_call,
};

static int aei_ftracer_init(struct trace_array *tr)
{
	struct aei_ftrace_record *rec = (struct aei_ftrace_record *)trace_buf;
	int i;

	for (i = 0; i < 2; i++) {
		rec = (struct aei_ftrace_record *)(trace_buf + i * PAGE_SIZE);
		rec->ip = MAGIC; /* store MAGIC number in first record */
	}

	tracing_start_cmdline_record();

	register_ftrace_function(&trace_ops);

	return 0;
}

static void aei_ftrace_reset(struct trace_array *tr)
{
	unregister_ftrace_function(&trace_ops);

	tracing_stop_cmdline_record();
}

static void aei_ftrace_start(struct trace_array *tr)
{
	tracing_reset_online_cpus(tr);
}

static struct tracer aei_ftracer __read_mostly = {
	.name		= "aei_ftrace",
	.init		= aei_ftracer_init,
	.reset		= aei_ftrace_reset,
	.start		= aei_ftrace_start,
	.wait_pipe	= poll_wait_pipe,
};

static void dump_trace_record(struct aei_ftrace_record *rec)
{
	char sym1[KSYM_SYMBOL_LEN], sym2[KSYM_SYMBOL_LEN];
	char irq_disabled, hardirq, softirq;

	irq_disabled = hardirq = softirq = '-';
	if (rec->ip & PTRACE_IRQ_DISABLED)
		irq_disabled = 'D';
	if (rec->parent_ip & HARDIRQ)
		hardirq = 'H';
	if (rec->parent_ip & SOFTIRQ)
		softirq = 'S';

	rec->ip = rec->ip & (~3);
	sprint_symbol(sym1, rec->ip);
	rec->parent_ip = rec->parent_ip & (~3);
	sprint_symbol(sym2, rec->parent_ip);
	printk("%c%c%c %s <- %s\n",
		irq_disabled, hardirq, softirq, sym1, sym2);
}

static void dump_ftrace_cpu(int cpu)
{
	struct aei_ftrace_record *rec;
	int count = PAGE_SIZE / sizeof(struct aei_ftrace_record);
	int first;
	int i;

	printk("cpu %d function trace: D(irq disabled), H(hardirq), S(softirq)\n", cpu);
	printk("--------------------------------------------------------------\n");
	rec = (struct aei_ftrace_record *)(trace_buf + cpu * PAGE_SIZE);
	if (rec->ip != MAGIC)
		return;
	first = rec->parent_ip;
	if (first < 0 || first >= count)
		return;

	for (i = first; i < count; i++)
		dump_trace_record(&rec[i]);
	for (i = 1; i < first; i++)
		dump_trace_record(&rec[i]);
	printk("\n");
}

static int proc_read_ftrace(char *buf, char **start,
                             off_t off, int count,
                             int *eof, void *data)
{
	int i;

	for (i = 0; i < 2; i++)
		dump_ftrace_cpu(i);

	return 0;
}

static void ftrace_proc_init(void)
{
       struct proc_dir_entry *entry;

       entry = create_proc_read_entry("aei_ftrace",
               0444, NULL,
               proc_read_ftrace,
               NULL);
       if (!entry)
               printk("AEI ftrace: /proc/aei_ftrace create fail\n");
}

/* 8k@100M */
static unsigned long ftrace_base = 0x6400000;
static unsigned long ftrace_size = 0x2000;
static int mem_reserved;

void __init aei_reserve_ftrace_mem(void)
{
	int ret;

	ret = reserve_bootmem(ftrace_base, ftrace_size, BOOTMEM_EXCLUSIVE);
	if (ret < 0) {
		printk(KERN_WARNING "AEI ftrace: mem reservation failed - "
			"memory is in use (0x%lx)\n", (unsigned long)ftrace_base);
		return;
	}

	printk("AEI ftrace: Reserving %ldKB of memory at %ldMB\n",
		(unsigned long)(ftrace_size >> 10),
		(unsigned long)(ftrace_base >> 20));

	mem_reserved = 1;
}

static int __init aei_ftrace_init(void)
{
	int ret;

	if (!mem_reserved) {
		printk("AEI ftrace: mem was not reserved\n");
		return 0;
	}

	trace_buf = ioremap(ftrace_base, ftrace_size);
	if (!trace_buf) {
		printk("AEI ftrace: ioremap fails in %s\n", __func__);
		return 0;
	}

        ret = register_tracer(&aei_ftracer);
	if (ret) {
		pr_err("AEI ftrace: aei_ftrace: failed to register tracer");
		return ret;
	}

	ftrace_proc_init();

	return 0;
}
late_initcall(aei_ftrace_init);
