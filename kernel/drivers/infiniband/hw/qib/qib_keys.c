

#include "qib.h"


int qib_alloc_lkey(struct qib_lkey_table *rkt, struct qib_mregion *mr)
{
	unsigned long flags;
	u32 r;
	u32 n;
	int ret;

	spin_lock_irqsave(&rkt->lock, flags);

	/* Find the next available LKEY */
	r = rkt->next;
	n = r;
	for (;;) {
		if (rkt->table[r] == NULL)
			break;
		r = (r + 1) & (rkt->max - 1);
		if (r == n) {
			spin_unlock_irqrestore(&rkt->lock, flags);
			ret = 0;
			goto bail;
		}
	}
	rkt->next = (r + 1) & (rkt->max - 1);
	/*
	 * Make sure lkey is never zero which is reserved to indicate an
	 * unrestricted LKEY.
	 */
	rkt->gen++;
	mr->lkey = (r << (32 - ib_qib_lkey_table_size)) |
		((((1 << (24 - ib_qib_lkey_table_size)) - 1) & rkt->gen)
		 << 8);
	if (mr->lkey == 0) {
		mr->lkey |= 1 << 8;
		rkt->gen++;
	}
	rkt->table[r] = mr;
	spin_unlock_irqrestore(&rkt->lock, flags);

	ret = 1;

bail:
	return ret;
}

int qib_free_lkey(struct qib_ibdev *dev, struct qib_mregion *mr)
{
	unsigned long flags;
	u32 lkey = mr->lkey;
	u32 r;
	int ret;

	spin_lock_irqsave(&dev->lk_table.lock, flags);
	if (lkey == 0) {
		if (dev->dma_mr && dev->dma_mr == mr) {
			ret = atomic_read(&dev->dma_mr->refcount);
			if (!ret)
				dev->dma_mr = NULL;
		} else
			ret = 0;
	} else {
		r = lkey >> (32 - ib_qib_lkey_table_size);
		ret = atomic_read(&dev->lk_table.table[r]->refcount);
		if (!ret)
			dev->lk_table.table[r] = NULL;
	}
	spin_unlock_irqrestore(&dev->lk_table.lock, flags);

	if (ret)
		ret = -EBUSY;
	return ret;
}

int qib_lkey_ok(struct qib_lkey_table *rkt, struct qib_pd *pd,
		struct qib_sge *isge, struct ib_sge *sge, int acc)
{
	struct qib_mregion *mr;
	unsigned n, m;
	size_t off;
	int ret = 0;
	unsigned long flags;

	/*
	 * We use LKEY == zero for kernel virtual addresses
	 * (see qib_get_dma_mr and qib_dma.c).
	 */
	spin_lock_irqsave(&rkt->lock, flags);
	if (sge->lkey == 0) {
		struct qib_ibdev *dev = to_idev(pd->ibpd.device);

		if (pd->user)
			goto bail;
		if (!dev->dma_mr)
			goto bail;
		atomic_inc(&dev->dma_mr->refcount);
		isge->mr = dev->dma_mr;
		isge->vaddr = (void *) sge->addr;
		isge->length = sge->length;
		isge->sge_length = sge->length;
		isge->m = 0;
		isge->n = 0;
		goto ok;
	}
	mr = rkt->table[(sge->lkey >> (32 - ib_qib_lkey_table_size))];
	if (unlikely(mr == NULL || mr->lkey != sge->lkey ||
		     mr->pd != &pd->ibpd))
		goto bail;

	off = sge->addr - mr->user_base;
	if (unlikely(sge->addr < mr->user_base ||
		     off + sge->length > mr->length ||
		     (mr->access_flags & acc) != acc))
		goto bail;

	off += mr->offset;
	m = 0;
	n = 0;
	while (off >= mr->map[m]->segs[n].length) {
		off -= mr->map[m]->segs[n].length;
		n++;
		if (n >= QIB_SEGSZ) {
			m++;
			n = 0;
		}
	}
	atomic_inc(&mr->refcount);
	isge->mr = mr;
	isge->vaddr = mr->map[m]->segs[n].vaddr + off;
	isge->length = mr->map[m]->segs[n].length - off;
	isge->sge_length = sge->length;
	isge->m = m;
	isge->n = n;
ok:
	ret = 1;
bail:
	spin_unlock_irqrestore(&rkt->lock, flags);
	return ret;
}

int qib_rkey_ok(struct qib_qp *qp, struct qib_sge *sge,
		u32 len, u64 vaddr, u32 rkey, int acc)
{
	struct qib_lkey_table *rkt = &to_idev(qp->ibqp.device)->lk_table;
	struct qib_mregion *mr;
	unsigned n, m;
	size_t off;
	int ret = 0;
	unsigned long flags;

	/*
	 * We use RKEY == zero for kernel virtual addresses
	 * (see qib_get_dma_mr and qib_dma.c).
	 */
	spin_lock_irqsave(&rkt->lock, flags);
	if (rkey == 0) {
		struct qib_pd *pd = to_ipd(qp->ibqp.pd);
		struct qib_ibdev *dev = to_idev(pd->ibpd.device);

		if (pd->user)
			goto bail;
		if (!dev->dma_mr)
			goto bail;
		atomic_inc(&dev->dma_mr->refcount);
		sge->mr = dev->dma_mr;
		sge->vaddr = (void *) vaddr;
		sge->length = len;
		sge->sge_length = len;
		sge->m = 0;
		sge->n = 0;
		goto ok;
	}

	mr = rkt->table[(rkey >> (32 - ib_qib_lkey_table_size))];
	if (unlikely(mr == NULL || mr->lkey != rkey || qp->ibqp.pd != mr->pd))
		goto bail;

	off = vaddr - mr->iova;
	if (unlikely(vaddr < mr->iova || off + len > mr->length ||
		     (mr->access_flags & acc) == 0))
		goto bail;

	off += mr->offset;
	m = 0;
	n = 0;
	while (off >= mr->map[m]->segs[n].length) {
		off -= mr->map[m]->segs[n].length;
		n++;
		if (n >= QIB_SEGSZ) {
			m++;
			n = 0;
		}
	}
	atomic_inc(&mr->refcount);
	sge->mr = mr;
	sge->vaddr = mr->map[m]->segs[n].vaddr + off;
	sge->length = mr->map[m]->segs[n].length - off;
	sge->sge_length = len;
	sge->m = m;
	sge->n = n;
ok:
	ret = 1;
bail:
	spin_unlock_irqrestore(&rkt->lock, flags);
	return ret;
}

int qib_fast_reg_mr(struct qib_qp *qp, struct ib_send_wr *wr)
{
	struct qib_lkey_table *rkt = &to_idev(qp->ibqp.device)->lk_table;
	struct qib_pd *pd = to_ipd(qp->ibqp.pd);
	struct qib_mregion *mr;
	u32 rkey = wr->wr.fast_reg.rkey;
	unsigned i, n, m;
	int ret = -EINVAL;
	unsigned long flags;
	u64 *page_list;
	size_t ps;

	spin_lock_irqsave(&rkt->lock, flags);
	if (pd->user || rkey == 0)
		goto bail;

	mr = rkt->table[(rkey >> (32 - ib_qib_lkey_table_size))];
	if (unlikely(mr == NULL || qp->ibqp.pd != mr->pd))
		goto bail;

	if (wr->wr.fast_reg.page_list_len > mr->max_segs)
		goto bail;

	ps = 1UL << wr->wr.fast_reg.page_shift;
	if (wr->wr.fast_reg.length > ps * wr->wr.fast_reg.page_list_len)
		goto bail;

	mr->user_base = wr->wr.fast_reg.iova_start;
	mr->iova = wr->wr.fast_reg.iova_start;
	mr->lkey = rkey;
	mr->length = wr->wr.fast_reg.length;
	mr->access_flags = wr->wr.fast_reg.access_flags;
	page_list = wr->wr.fast_reg.page_list->page_list;
	m = 0;
	n = 0;
	for (i = 0; i < wr->wr.fast_reg.page_list_len; i++) {
		mr->map[m]->segs[n].vaddr = (void *) page_list[i];
		mr->map[m]->segs[n].length = ps;
		if (++n == QIB_SEGSZ) {
			m++;
			n = 0;
		}
	}

	ret = 0;
bail:
	spin_unlock_irqrestore(&rkt->lock, flags);
	return ret;
}
