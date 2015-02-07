

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <net/pkt_sched.h>

static int blackhole_enqueue(struct sk_buff *skb, struct Qdisc *sch)
{
	qdisc_drop(skb, sch);
	return NET_XMIT_SUCCESS;
}

static struct sk_buff *blackhole_dequeue(struct Qdisc *sch)
{
	return NULL;
}

static struct Qdisc_ops blackhole_qdisc_ops __read_mostly = {
	.id		= "blackhole",
	.priv_size	= 0,
	.enqueue	= blackhole_enqueue,
	.dequeue	= blackhole_dequeue,
	.peek		= blackhole_dequeue,
	.owner		= THIS_MODULE,
};

static int __init blackhole_module_init(void)
{
	return register_qdisc(&blackhole_qdisc_ops);
}

static void __exit blackhole_module_exit(void)
{
	unregister_qdisc(&blackhole_qdisc_ops);
}

module_init(blackhole_module_init)
module_exit(blackhole_module_exit)

MODULE_LICENSE("GPL");