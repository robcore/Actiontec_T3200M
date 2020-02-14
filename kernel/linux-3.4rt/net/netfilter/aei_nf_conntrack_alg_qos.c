#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/net.h>
#include <linux/netlink.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/hardirq.h>
#include <linux/skbuff.h>
#include <linux/netfilter.h>
#include <linux/netfilter/nf_conntrack_common.h>
#include <linux/netfilter/nf_conntrack_tuple_common.h>

#include <net/sock.h>
#include <net/net_namespace.h>
#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_tuple.h>

#include <net/netfilter/aei_nf_conntrack_alg_qos.h>

MODULE_LICENSE("Actiontec");
MODULE_AUTHOR("Paul <ydu@actiontec.com>");
MODULE_DESCRIPTION("Kernel module for ALG QoS");
MODULE_ALIAS("aei_nf_conntrack_alg_qos");

static unsigned int ssk_pid = 0;

static struct proc_dir_entry *alg_qos_proc_dir;
static struct sock *alg_qos_sock = NULL;

static void aei_alg_qos_send_netlink_msg(void *data)
{
    struct sk_buff *skb = NULL;
    struct nlmsghdr *msg_hdr = NULL;
    unsigned int payload_len = sizeof(struct nlmsghdr);
    int msg_data_len;

    if (ssk_pid == 0)
    {
        printk(KERN_ERR "Don't know where to send the message, no received process initialized\n");
        return;
    }

    if (alg_qos_sock == NULL)
    {
        alg_qos_sock = netlink_kernel_create(&init_net, NETLINK_ALG_QOS, 0, NULL, NULL, THIS_MODULE);
        if (!alg_qos_sock)
        {
            printk(KERN_ERR "Failed to create a netlink socket for ALG QoS\n");
            return;
        }
    }

    msg_data_len = sizeof(alg_qos_msg_t);

    payload_len += msg_data_len;
    payload_len = NLMSG_SPACE(payload_len);

    if (in_atomic())
    {
        skb = alloc_skb(payload_len, GFP_ATOMIC);
    }
    else
    {
        skb = alloc_skb(payload_len, GFP_KERNEL);
    }

    if (!skb)
    {
        printk(KERN_ERR "Failed to alloc skb in %s", __FUNCTION__);
        return;
    }

    msg_hdr = (struct nlmsghdr *)skb->data;
    msg_hdr->nlmsg_type = 0;
    msg_hdr->nlmsg_pid = 0;   /* from kernel */
    msg_hdr->nlmsg_len = payload_len;
    msg_hdr->nlmsg_flags = 0;
    if (data)
    {
        memcpy(NLMSG_DATA(msg_hdr), data, msg_data_len);
    }

    skb->len = payload_len;
    NETLINK_CB(skb).pid = 0; /* from kernel */
    NETLINK_CB(skb).dst_group = 0;

    netlink_unicast(alg_qos_sock, skb, ssk_pid, MSG_DONTWAIT);
}

static ssize_t aei_alg_qos_ssk_pid_write(struct file *file, const char *buf, size_t length, loff_t *offset)
{
    char temp[32];
    char *endp;

    if (length > 32)
    {
        printk("length is too long\n");
        return length;
    }

    memset(temp, 0, sizeof(temp));
    copy_from_user(temp, buf, length - 1);
    ssk_pid = simple_strtoul(temp, &endp, 10);

    printk("aei_alg_qos_ssk_pid_write: ssk_pid:%u\n", ssk_pid);

    return length;
}

static int aei_alg_qos_ssk_pid_show(struct seq_file *m, void *v)
{
    seq_printf(m, "aei_alg_qos_ssk_pid_show: ssk_pid %u\n", ssk_pid);
    return 0;
}

static int aei_alg_qos_ssk_pid_open(struct inode *inode, struct file *file)
{
    return single_open(file, &aei_alg_qos_ssk_pid_show, NULL);
}

static const struct file_operations aei_alg_qos_ssk_pid_file_ops =
{
    .owner   = THIS_MODULE,
    .open    = aei_alg_qos_ssk_pid_open,
    .read    = seq_read,
    .write   = aei_alg_qos_ssk_pid_write,
    .llseek  = seq_lseek,
    .release = single_release,
};

void aei_alg_qos_handle_conn(alg_qos_action_e action, alg_qos_proto_e proto, struct nf_conntrack_tuple *tuple)
{
    alg_qos_msg_t msg;

    if (tuple == NULL)
    {
        return;
    }

    memset(&msg, 0, sizeof(alg_qos_msg_t));

    msg.action = action;
    msg.proto = proto;
    msg.ip_proto = tuple->dst.protonum;
    msg.src_ip = tuple->src.u3.ip;
    msg.src_port = ntohs(tuple->src.u.udp.port);
    msg.dest_ip = tuple->dst.u3.ip;
    msg.dest_port = ntohs(tuple->dst.u.udp.port);

    aei_alg_qos_send_netlink_msg((void *)&msg);

    return;
}
EXPORT_SYMBOL(aei_alg_qos_handle_conn);

static void __exit aei_nf_conntrack_alg_qos_fini(void)
{
    remove_proc_entry("ssk_pid", alg_qos_proc_dir);
    remove_proc_entry("alg_qos", proc_net_netfilter);

    if (alg_qos_sock)
    {
        sock_release(alg_qos_sock->sk_socket);
    }
}

static int __init aei_nf_conntrack_alg_qos_init(void)
{
    struct proc_dir_entry *p;
    int rc = -ENOMEM;

    alg_qos_proc_dir = proc_mkdir("alg_qos", proc_net_netfilter);
    if (!alg_qos_proc_dir)
        return rc;

    p = proc_create("ssk_pid", S_IRUGO,
                    alg_qos_proc_dir, &aei_alg_qos_ssk_pid_file_ops);
    if (!p)
        goto out_pid;

    alg_qos_sock = netlink_kernel_create(&init_net, NETLINK_ALG_QOS, 0, NULL, NULL, THIS_MODULE);
    if (!alg_qos_sock)
    {
        printk(KERN_ERR "Failed to create a netlink socket for ALG QoS\n");
    }

    return 0;

out_pid:
    remove_proc_entry("alg_qos", proc_net_netfilter);

    return rc;
}

module_init(aei_nf_conntrack_alg_qos_init);
module_exit(aei_nf_conntrack_alg_qos_fini);
