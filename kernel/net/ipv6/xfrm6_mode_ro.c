

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/spinlock.h>
#include <linux/stringify.h>
#include <linux/time.h>
#include <net/ipv6.h>
#include <net/xfrm.h>

static int xfrm6_ro_output(struct xfrm_state *x, struct sk_buff *skb)
{
	struct ipv6hdr *iph;
	u8 *prevhdr;
	int hdr_len;

	iph = ipv6_hdr(skb);

	hdr_len = x->type->hdr_offset(x, skb, &prevhdr);
	skb_set_mac_header(skb, (prevhdr - x->props.header_len) - skb->data);
	skb_set_network_header(skb, -x->props.header_len);
	skb->transport_header = skb->network_header + hdr_len;
	__skb_pull(skb, hdr_len);
	memmove(ipv6_hdr(skb), iph, hdr_len);

	x->lastused = get_seconds();

	return 0;
}

static struct xfrm_mode xfrm6_ro_mode = {
	.output = xfrm6_ro_output,
	.owner = THIS_MODULE,
	.encap = XFRM_MODE_ROUTEOPTIMIZATION,
};

static int __init xfrm6_ro_init(void)
{
	return xfrm_register_mode(&xfrm6_ro_mode, AF_INET6);
}

static void __exit xfrm6_ro_exit(void)
{
	int err;

	err = xfrm_unregister_mode(&xfrm6_ro_mode, AF_INET6);
	BUG_ON(err);
}

module_init(xfrm6_ro_init);
module_exit(xfrm6_ro_exit);
MODULE_LICENSE("GPL");
MODULE_ALIAS_XFRM_MODE(AF_INET6, XFRM_MODE_ROUTEOPTIMIZATION);
