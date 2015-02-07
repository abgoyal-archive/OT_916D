

#include <linux/string.h>
#include <linux/slab.h>
#include "packet_history.h"
#include "../../dccp.h"

struct tfrc_tx_hist_entry {
	struct tfrc_tx_hist_entry *next;
	u64			  seqno;
	ktime_t			  stamp;
};

static struct kmem_cache *tfrc_tx_hist_slab;

int __init tfrc_tx_packet_history_init(void)
{
	tfrc_tx_hist_slab = kmem_cache_create("tfrc_tx_hist",
					      sizeof(struct tfrc_tx_hist_entry),
					      0, SLAB_HWCACHE_ALIGN, NULL);
	return tfrc_tx_hist_slab == NULL ? -ENOBUFS : 0;
}

void tfrc_tx_packet_history_exit(void)
{
	if (tfrc_tx_hist_slab != NULL) {
		kmem_cache_destroy(tfrc_tx_hist_slab);
		tfrc_tx_hist_slab = NULL;
	}
}

static struct tfrc_tx_hist_entry *
	tfrc_tx_hist_find_entry(struct tfrc_tx_hist_entry *head, u64 seqno)
{
	while (head != NULL && head->seqno != seqno)
		head = head->next;

	return head;
}

int tfrc_tx_hist_add(struct tfrc_tx_hist_entry **headp, u64 seqno)
{
	struct tfrc_tx_hist_entry *entry = kmem_cache_alloc(tfrc_tx_hist_slab, gfp_any());

	if (entry == NULL)
		return -ENOBUFS;
	entry->seqno = seqno;
	entry->stamp = ktime_get_real();
	entry->next  = *headp;
	*headp	     = entry;
	return 0;
}

void tfrc_tx_hist_purge(struct tfrc_tx_hist_entry **headp)
{
	struct tfrc_tx_hist_entry *head = *headp;

	while (head != NULL) {
		struct tfrc_tx_hist_entry *next = head->next;

		kmem_cache_free(tfrc_tx_hist_slab, head);
		head = next;
	}

	*headp = NULL;
}

u32 tfrc_tx_hist_rtt(struct tfrc_tx_hist_entry *head, const u64 seqno,
		     const ktime_t now)
{
	u32 rtt = 0;
	struct tfrc_tx_hist_entry *packet = tfrc_tx_hist_find_entry(head, seqno);

	if (packet != NULL) {
		rtt = ktime_us_delta(now, packet->stamp);
		/*
		 * Garbage-collect older (irrelevant) entries:
		 */
		tfrc_tx_hist_purge(&packet->next);
	}

	return rtt;
}


static struct kmem_cache *tfrc_rx_hist_slab;

int __init tfrc_rx_packet_history_init(void)
{
	tfrc_rx_hist_slab = kmem_cache_create("tfrc_rxh_cache",
					      sizeof(struct tfrc_rx_hist_entry),
					      0, SLAB_HWCACHE_ALIGN, NULL);
	return tfrc_rx_hist_slab == NULL ? -ENOBUFS : 0;
}

void tfrc_rx_packet_history_exit(void)
{
	if (tfrc_rx_hist_slab != NULL) {
		kmem_cache_destroy(tfrc_rx_hist_slab);
		tfrc_rx_hist_slab = NULL;
	}
}

static inline void tfrc_rx_hist_entry_from_skb(struct tfrc_rx_hist_entry *entry,
					       const struct sk_buff *skb,
					       const u64 ndp)
{
	const struct dccp_hdr *dh = dccp_hdr(skb);

	entry->tfrchrx_seqno = DCCP_SKB_CB(skb)->dccpd_seq;
	entry->tfrchrx_ccval = dh->dccph_ccval;
	entry->tfrchrx_type  = dh->dccph_type;
	entry->tfrchrx_ndp   = ndp;
	entry->tfrchrx_tstamp = ktime_get_real();
}

void tfrc_rx_hist_add_packet(struct tfrc_rx_hist *h,
			     const struct sk_buff *skb,
			     const u64 ndp)
{
	struct tfrc_rx_hist_entry *entry = tfrc_rx_hist_last_rcv(h);

	tfrc_rx_hist_entry_from_skb(entry, skb, ndp);
}

/* has the packet contained in skb been seen before? */
int tfrc_rx_hist_duplicate(struct tfrc_rx_hist *h, struct sk_buff *skb)
{
	const u64 seq = DCCP_SKB_CB(skb)->dccpd_seq;
	int i;

	if (dccp_delta_seqno(tfrc_rx_hist_loss_prev(h)->tfrchrx_seqno, seq) <= 0)
		return 1;

	for (i = 1; i <= h->loss_count; i++)
		if (tfrc_rx_hist_entry(h, i)->tfrchrx_seqno == seq)
			return 1;

	return 0;
}

static void tfrc_rx_hist_swap(struct tfrc_rx_hist *h, const u8 a, const u8 b)
{
	const u8 idx_a = tfrc_rx_hist_index(h, a),
		 idx_b = tfrc_rx_hist_index(h, b);
	struct tfrc_rx_hist_entry *tmp = h->ring[idx_a];

	h->ring[idx_a] = h->ring[idx_b];
	h->ring[idx_b] = tmp;
}

static void __do_track_loss(struct tfrc_rx_hist *h, struct sk_buff *skb, u64 n1)
{
	u64 s0 = tfrc_rx_hist_loss_prev(h)->tfrchrx_seqno,
	    s1 = DCCP_SKB_CB(skb)->dccpd_seq;

	if (!dccp_loss_free(s0, s1, n1)) {	/* gap between S0 and S1 */
		h->loss_count = 1;
		tfrc_rx_hist_entry_from_skb(tfrc_rx_hist_entry(h, 1), skb, n1);
	}
}

static void __one_after_loss(struct tfrc_rx_hist *h, struct sk_buff *skb, u32 n2)
{
	u64 s0 = tfrc_rx_hist_loss_prev(h)->tfrchrx_seqno,
	    s1 = tfrc_rx_hist_entry(h, 1)->tfrchrx_seqno,
	    s2 = DCCP_SKB_CB(skb)->dccpd_seq;

	if (likely(dccp_delta_seqno(s1, s2) > 0)) {	/* S1  <  S2 */
		h->loss_count = 2;
		tfrc_rx_hist_entry_from_skb(tfrc_rx_hist_entry(h, 2), skb, n2);
		return;
	}

	/* S0  <  S2  <  S1 */

	if (dccp_loss_free(s0, s2, n2)) {
		u64 n1 = tfrc_rx_hist_entry(h, 1)->tfrchrx_ndp;

		if (dccp_loss_free(s2, s1, n1)) {
			/* hole is filled: S0, S2, and S1 are consecutive */
			h->loss_count = 0;
			h->loss_start = tfrc_rx_hist_index(h, 1);
		} else
			/* gap between S2 and S1: just update loss_prev */
			tfrc_rx_hist_entry_from_skb(tfrc_rx_hist_loss_prev(h), skb, n2);

	} else {	/* gap between S0 and S2 */
		/*
		 * Reorder history to insert S2 between S0 and S1
		 */
		tfrc_rx_hist_swap(h, 0, 3);
		h->loss_start = tfrc_rx_hist_index(h, 3);
		tfrc_rx_hist_entry_from_skb(tfrc_rx_hist_entry(h, 1), skb, n2);
		h->loss_count = 2;
	}
}

/* return 1 if a new loss event has been identified */
static int __two_after_loss(struct tfrc_rx_hist *h, struct sk_buff *skb, u32 n3)
{
	u64 s0 = tfrc_rx_hist_loss_prev(h)->tfrchrx_seqno,
	    s1 = tfrc_rx_hist_entry(h, 1)->tfrchrx_seqno,
	    s2 = tfrc_rx_hist_entry(h, 2)->tfrchrx_seqno,
	    s3 = DCCP_SKB_CB(skb)->dccpd_seq;

	if (likely(dccp_delta_seqno(s2, s3) > 0)) {	/* S2  <  S3 */
		h->loss_count = 3;
		tfrc_rx_hist_entry_from_skb(tfrc_rx_hist_entry(h, 3), skb, n3);
		return 1;
	}

	/* S3  <  S2 */

	if (dccp_delta_seqno(s1, s3) > 0) {		/* S1  <  S3  <  S2 */
		/*
		 * Reorder history to insert S3 between S1 and S2
		 */
		tfrc_rx_hist_swap(h, 2, 3);
		tfrc_rx_hist_entry_from_skb(tfrc_rx_hist_entry(h, 2), skb, n3);
		h->loss_count = 3;
		return 1;
	}

	/* S0  <  S3  <  S1 */

	if (dccp_loss_free(s0, s3, n3)) {
		u64 n1 = tfrc_rx_hist_entry(h, 1)->tfrchrx_ndp;

		if (dccp_loss_free(s3, s1, n1)) {
			/* hole between S0 and S1 filled by S3 */
			u64 n2 = tfrc_rx_hist_entry(h, 2)->tfrchrx_ndp;

			if (dccp_loss_free(s1, s2, n2)) {
				/* entire hole filled by S0, S3, S1, S2 */
				h->loss_start = tfrc_rx_hist_index(h, 2);
				h->loss_count = 0;
			} else {
				/* gap remains between S1 and S2 */
				h->loss_start = tfrc_rx_hist_index(h, 1);
				h->loss_count = 1;
			}

		} else /* gap exists between S3 and S1, loss_count stays at 2 */
			tfrc_rx_hist_entry_from_skb(tfrc_rx_hist_loss_prev(h), skb, n3);

		return 0;
	}

	/*
	 * The remaining case:  S0  <  S3  <  S1  <  S2;  gap between S0 and S3
	 * Reorder history to insert S3 between S0 and S1.
	 */
	tfrc_rx_hist_swap(h, 0, 3);
	h->loss_start = tfrc_rx_hist_index(h, 3);
	tfrc_rx_hist_entry_from_skb(tfrc_rx_hist_entry(h, 1), skb, n3);
	h->loss_count = 3;

	return 1;
}

/* recycle RX history records to continue loss detection if necessary */
static void __three_after_loss(struct tfrc_rx_hist *h)
{
	/*
	 * At this stage we know already that there is a gap between S0 and S1
	 * (since S0 was the highest sequence number received before detecting
	 * the loss). To recycle the loss record, it is	thus only necessary to
	 * check for other possible gaps between S1/S2 and between S2/S3.
	 */
	u64 s1 = tfrc_rx_hist_entry(h, 1)->tfrchrx_seqno,
	    s2 = tfrc_rx_hist_entry(h, 2)->tfrchrx_seqno,
	    s3 = tfrc_rx_hist_entry(h, 3)->tfrchrx_seqno;
	u64 n2 = tfrc_rx_hist_entry(h, 2)->tfrchrx_ndp,
	    n3 = tfrc_rx_hist_entry(h, 3)->tfrchrx_ndp;

	if (dccp_loss_free(s1, s2, n2)) {

		if (dccp_loss_free(s2, s3, n3)) {
			/* no gap between S2 and S3: entire hole is filled */
			h->loss_start = tfrc_rx_hist_index(h, 3);
			h->loss_count = 0;
		} else {
			/* gap between S2 and S3 */
			h->loss_start = tfrc_rx_hist_index(h, 2);
			h->loss_count = 1;
		}

	} else {	/* gap between S1 and S2 */
		h->loss_start = tfrc_rx_hist_index(h, 1);
		h->loss_count = 2;
	}
}

int tfrc_rx_handle_loss(struct tfrc_rx_hist *h,
			struct tfrc_loss_hist *lh,
			struct sk_buff *skb, const u64 ndp,
			u32 (*calc_first_li)(struct sock *), struct sock *sk)
{
	int is_new_loss = 0;

	if (h->loss_count == 0) {
		__do_track_loss(h, skb, ndp);
	} else if (h->loss_count == 1) {
		__one_after_loss(h, skb, ndp);
	} else if (h->loss_count != 2) {
		DCCP_BUG("invalid loss_count %d", h->loss_count);
	} else if (__two_after_loss(h, skb, ndp)) {
		/*
		 * Update Loss Interval database and recycle RX records
		 */
		is_new_loss = tfrc_lh_interval_add(lh, h, calc_first_li, sk);
		__three_after_loss(h);
	}
	return is_new_loss;
}

int tfrc_rx_hist_alloc(struct tfrc_rx_hist *h)
{
	int i;

	for (i = 0; i <= TFRC_NDUPACK; i++) {
		h->ring[i] = kmem_cache_alloc(tfrc_rx_hist_slab, GFP_ATOMIC);
		if (h->ring[i] == NULL)
			goto out_free;
	}

	h->loss_count = h->loss_start = 0;
	return 0;

out_free:
	while (i-- != 0) {
		kmem_cache_free(tfrc_rx_hist_slab, h->ring[i]);
		h->ring[i] = NULL;
	}
	return -ENOBUFS;
}

void tfrc_rx_hist_purge(struct tfrc_rx_hist *h)
{
	int i;

	for (i = 0; i <= TFRC_NDUPACK; ++i)
		if (h->ring[i] != NULL) {
			kmem_cache_free(tfrc_rx_hist_slab, h->ring[i]);
			h->ring[i] = NULL;
		}
}

static inline struct tfrc_rx_hist_entry *
			tfrc_rx_hist_rtt_last_s(const struct tfrc_rx_hist *h)
{
	return h->ring[0];
}

static inline struct tfrc_rx_hist_entry *
			tfrc_rx_hist_rtt_prev_s(const struct tfrc_rx_hist *h)
{
	return h->ring[h->rtt_sample_prev];
}

u32 tfrc_rx_hist_sample_rtt(struct tfrc_rx_hist *h, const struct sk_buff *skb)
{
	u32 sample = 0,
	    delta_v = SUB16(dccp_hdr(skb)->dccph_ccval,
			    tfrc_rx_hist_rtt_last_s(h)->tfrchrx_ccval);

	if (delta_v < 1 || delta_v > 4) {	/* unsuitable CCVal delta */
		if (h->rtt_sample_prev == 2) {	/* previous candidate stored */
			sample = SUB16(tfrc_rx_hist_rtt_prev_s(h)->tfrchrx_ccval,
				       tfrc_rx_hist_rtt_last_s(h)->tfrchrx_ccval);
			if (sample)
				sample = 4 / sample *
				         ktime_us_delta(tfrc_rx_hist_rtt_prev_s(h)->tfrchrx_tstamp,
							tfrc_rx_hist_rtt_last_s(h)->tfrchrx_tstamp);
			else    /*
				 * FIXME: This condition is in principle not
				 * possible but occurs when CCID is used for
				 * two-way data traffic. I have tried to trace
				 * it, but the cause does not seem to be here.
				 */
				DCCP_BUG("please report to dccp@vger.kernel.org"
					 " => prev = %u, last = %u",
					 tfrc_rx_hist_rtt_prev_s(h)->tfrchrx_ccval,
					 tfrc_rx_hist_rtt_last_s(h)->tfrchrx_ccval);
		} else if (delta_v < 1) {
			h->rtt_sample_prev = 1;
			goto keep_ref_for_next_time;
		}

	} else if (delta_v == 4) /* optimal match */
		sample = ktime_to_us(net_timedelta(tfrc_rx_hist_rtt_last_s(h)->tfrchrx_tstamp));
	else {			 /* suboptimal match */
		h->rtt_sample_prev = 2;
		goto keep_ref_for_next_time;
	}

	if (unlikely(sample > DCCP_SANE_RTT_MAX)) {
		DCCP_WARN("RTT sample %u too large, using max\n", sample);
		sample = DCCP_SANE_RTT_MAX;
	}

	h->rtt_sample_prev = 0;	       /* use current entry as next reference */
keep_ref_for_next_time:

	return sample;
}
