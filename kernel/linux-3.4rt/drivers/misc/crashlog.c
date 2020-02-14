/*
 * bowen <bwang@actiontec.com>
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/console.h>
#include <linux/vmalloc.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/mtd/mtd.h>
#include <linux/proc_fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/utsname.h>
#include <board.h>
#include <bcmTag.h>

#define OOPS_BUFFER_SIZE 4096
#define MAX_CRASHLOG_FILE_SIZE 1024 *256
#define CRASHLOG_FILE_NAME "/data/crashlog"

int act_panic_on_oops;
EXPORT_SYMBOL(act_panic_on_oops);

static struct oops_context {
	char path[50];
	unsigned char *oops_buf;
	/* writecount and disabling ready are spin lock protected */
	spinlock_t writecount_lock;
	int ready;
	int writecount;
	int is_clear_log;
} oops_cxt;

extern PFILE_TAG kerSysImageTagGet(void);

static int read_file(const char *path, char *buf, size_t buf_len)
{
	struct file *fp;
	loff_t pos;
	int ret;
	mm_segment_t fs;

	if (!path || !buf)
		return -1;

	fp = filp_open(path, O_RDONLY, 0);
	if (IS_ERR(fp))
		return -1;

	fs = get_fs(); 
	set_fs(KERNEL_DS);
	memset(buf, 0, buf_len);
	pos = 0;
	ret = vfs_read(fp, buf, buf_len, &pos);
	if (ret < 0) {
		ret = -1;
		goto out;
	}

out:
	filp_close(fp, NULL);
	set_fs(fs);
	return ret;
}

static void dump_firmware_version_info(void)
{
#if 0
	PFILE_TAG pTag = (PFILE_TAG)NULL;

	pTag = kerSysImageTagGet();
	if (pTag) {
		printk(KERN_ERR "\n*********firmware_version:***********\n");
		printk("signature(%s),version(%s),version2(%s),imageSequence(%s)\n",
		     pTag->signiture_1, pTag->signiture_2,
		     pTag->imageVersion, pTag->imageSequence);
		printk(KERN_ERR "**************end*************\n");
	}
#else
        char firmware_version[512];
        int ret;

        ret = read_file("/tmp/firmware_info",
	    firmware_version, sizeof(firmware_version));
        if (ret <= 0)
                return;
	firmware_version[ret] = '\0';
	printk(KERN_ERR "\n*********firmware_version:***********\n");
	printk(KERN_ERR "%s\n", firmware_version);
	printk(KERN_ERR "**************end*************\n");
#endif
}

static void dump_sys_version_info(void)
{
	char buf[512];
	int n;
	n = snprintf(buf, sizeof(buf), linux_proc_banner,
	           utsname()->sysname,
	           utsname()->release,
	           utsname()->version);
	buf[n] = '\0';
	printk(KERN_ERR "\n****system_version****\n%s\n*****end*****\n", buf);
}

static void dump_cpu_info(void)
{
	char tmp[512];
	char buf[512];
	int n;
	char *p;
	int ret, i, j;

	ret = read_file("/proc/stat", tmp, sizeof(tmp));
	if (ret < 0) {
		printk(KERN_DEBUG "dump_cpu_info: read_proc_file failed\n");
		return;
	}

	tmp[ret - 1] = '\0';
	p = tmp;
	n = snprintf(buf, sizeof(buf),
	    "     user system idle iowait irq softirq\n");
	for (i = 0; i < 5; i++) {
		p = strstr(p, "cpu");
		if (!p)
			break;
		for (j = 0; j < 7; j++) {
			while (*p !=' ' && *p != '\0')
				buf[n++] = *p++;
			while (*p == ' ' && *p != '\0')
				buf[n++] = *p++;
			if (*p == '\0')
				goto out;
		}
		buf[n++] = '\n';
	}
out:
	buf[n] = '\0';
	printk(KERN_ERR "\n******cpuinfo******\n%s\n******end******\n", buf);
}

static void dump_mem_info(void)
{
	char buf[1500];
	int ret;

	ret = read_file("/proc/meminfo", buf, sizeof(buf));
	if (ret <= 0)
		return;
	buf[ret] = '\0';
	printk(KERN_ERR "\n******meminfo******");
	printk(KERN_ERR "\n%s\n", buf);
	printk(KERN_ERR "********end********\n\n");
}

int act_dump_sysinfo(void)
{
	dump_firmware_version_info();
	dump_sys_version_info();
	dump_cpu_info();
	dump_mem_info();

	return 0;
}

EXPORT_SYMBOL(act_dump_sysinfo);

static int log_write(struct oops_context *cxt);

static inline int crash_log_size(struct file *fp)
{
	struct inode *inode;
	loff_t fsize;

	inode = fp->f_dentry->d_inode; 
	fsize = inode->i_size; 
	return (int)fsize; 
}

static void oops_write(struct oops_context *cxt)
{
	log_write(cxt);
	cxt->writecount = 0;
	cxt->ready = 1;
}

static void oops_console_sync(void)
{
	struct oops_context *cxt = &oops_cxt;
	unsigned long flags;

	if (!cxt->ready || cxt->writecount == 0)
		return;
	/*
	 *  Once ready is 0 and we've held the lock no further writes to the
	 *  buffer will happen
	 */
	spin_lock_irqsave(&cxt->writecount_lock, flags);
	if (!cxt->ready) {
		spin_unlock_irqrestore(&cxt->writecount_lock, flags);
		return;
	}
	cxt->ready = 0;
	spin_unlock_irqrestore(&cxt->writecount_lock, flags);

	oops_write(cxt);
}

static int log_write(struct oops_context *cxt)
{
	struct file *fp;
	int ret, log_size;
	mm_segment_t fs;
	int write_len, len;

	if (!cxt || cxt->writecount == 0)
		return 0;

	if (cxt->is_clear_log) {
		fp = filp_open(cxt->path, O_CREAT | O_RDWR |O_TRUNC, 0666);
		if (IS_ERR(fp))
			return PTR_ERR(fp);
		cxt->is_clear_log = 0;
	} else {
		fp = filp_open(cxt->path, O_CREAT | O_RDWR | O_APPEND, 0666);
		if (IS_ERR(fp))
			return PTR_ERR(fp);
	}

	/*if log_size is greater than MAX_CRASHLOG_FILE_SIZE, just return 0*/
	log_size = crash_log_size(fp);
	if (log_size > MAX_CRASHLOG_FILE_SIZE)
		return 0;

	fs = get_fs(); 
	set_fs(KERNEL_DS);
	write_len = cxt->writecount;
	len = 0;
	while (write_len > 0) {
		ret = vfs_write(fp, cxt->oops_buf + len, write_len, &fp->f_pos);
		if (ret < 0)
			break;
		write_len -= ret;
		len += ret;
	}
	if (act_panic_on_oops)
		vfs_fsync(fp, 0);

	filp_close(fp, NULL);
	set_fs(fs);
	return len;
}

static void
oops_console_write(struct console *co, const char *s, unsigned int count)
{
	struct oops_context *cxt = co->data;
	unsigned long flags;

	if (!oops_in_progress) {
		oops_console_sync();
		return;
	}

	if (!cxt->ready)
		return;

	/* Locking on writecount ensures sequential writes to the buffer */
	spin_lock_irqsave(&cxt->writecount_lock, flags);

	/* Check ready status didn't change whilst waiting for the lock */
	if (!cxt->ready) {
		spin_unlock_irqrestore(&cxt->writecount_lock, flags);
		return;
	}

	if ((count + cxt->writecount) > OOPS_BUFFER_SIZE)
		count = OOPS_BUFFER_SIZE - cxt->writecount;

	memcpy(cxt->oops_buf + cxt->writecount, s, count);
	cxt->writecount += count;

	spin_unlock_irqrestore(&cxt->writecount_lock, flags);
	if (act_panic_on_oops) {
		oops_console_sync();
	}

	if (cxt->writecount >= OOPS_BUFFER_SIZE - 1024)
		oops_console_sync();
}

static int __init oops_console_setup(struct console *co, char *options)
{
	return 0;
}

static struct console oops_console = {
	.name		= "clog",
	.write		= oops_console_write,
	.setup		= oops_console_setup,
	.unblank	= oops_console_sync,
	.index		= -1,
	.data		= &oops_cxt,
};

static int __init oops_console_init(void)
{
	struct oops_context *cxt = &oops_cxt;

	cxt->oops_buf = vmalloc(OOPS_BUFFER_SIZE);
	strncpy(cxt->path, CRASHLOG_FILE_NAME, sizeof(cxt->path));
	cxt->writecount = 0;
	cxt->is_clear_log = 1;
	cxt->ready = 1;
	spin_lock_init(&cxt->writecount_lock);

	if (!cxt->oops_buf) {
		printk(KERN_ERR "Failed to allocate mtdoops buffer workspace\n");
		return -ENOMEM;
	}

	register_console(&oops_console);

	return 0;
}

static void __exit oops_console_exit(void)
{
	struct oops_context *cxt = &oops_cxt;

	unregister_console(&oops_console);
	if (cxt->oops_buf)
		vfree(cxt->oops_buf);
}

subsys_initcall(oops_console_init);
module_exit(oops_console_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("bowen <bwang@actiontec.com>");
MODULE_DESCRIPTION("MTD Oops/Panic console logger/driver");
