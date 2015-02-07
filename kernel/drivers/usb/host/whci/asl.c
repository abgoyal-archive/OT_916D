
#include <linux/kernel.h>
#include <linux/gfp.h>
#include <linux/dma-mapping.h>
#include <linux/uwb/umc.h>
#include <linux/usb.h>

#include "../../wusbcore/wusbhc.h"

#include "whcd.h"

static void qset_get_next_prev(struct whc *whc, struct whc_qset *qset,
			       struct whc_qset **next, struct whc_qset **prev)
{
	struct list_head *n, *p;

	BUG_ON(list_empty(&whc->async_list));

	n = qset->list_node.next;
	if (n == &whc->async_list)
		n = n->next;
	p = qset->list_node.prev;
	if (p == &whc->async_list)
		p = p->prev;

	*next = container_of(n, struct whc_qset, list_node);
	*prev = container_of(p, struct whc_qset, list_node);

}

static void asl_qset_insert_begin(struct whc *whc, struct whc_qset *qset)
{
	list_move(&qset->list_node, &whc->async_list);
	qset->in_sw_list = true;
}

static void asl_qset_insert(struct whc *whc, struct whc_qset *qset)
{
	struct whc_qset *next, *prev;

	qset_clear(whc, qset);

	/* Link into ASL. */
	qset_get_next_prev(whc, qset, &next, &prev);
	whc_qset_set_link_ptr(&qset->qh.link, next->qset_dma);
	whc_qset_set_link_ptr(&prev->qh.link, qset->qset_dma);
	qset->in_hw_list = true;
}

static void asl_qset_remove(struct whc *whc, struct whc_qset *qset)
{
	struct whc_qset *prev, *next;

	qset_get_next_prev(whc, qset, &next, &prev);

	list_move(&qset->list_node, &whc->async_removed_list);
	qset->in_sw_list = false;

	/*
	 * No more qsets in the ASL?  The caller must stop the ASL as
	 * it's no longer valid.
	 */
	if (list_empty(&whc->async_list))
		return;

	/* Remove from ASL. */
	whc_qset_set_link_ptr(&prev->qh.link, next->qset_dma);
	qset->in_hw_list = false;
}

static uint32_t process_qset(struct whc *whc, struct whc_qset *qset)
{
	enum whc_update update = 0;
	uint32_t status = 0;

	while (qset->ntds) {
		struct whc_qtd *td;
		int t;

		t = qset->td_start;
		td = &qset->qtd[qset->td_start];
		status = le32_to_cpu(td->status);

		/*
		 * Nothing to do with a still active qTD.
		 */
		if (status & QTD_STS_ACTIVE)
			break;

		if (status & QTD_STS_HALTED) {
			/* Ug, an error. */
			process_halted_qtd(whc, qset, td);
			/* A halted qTD always triggers an update
			   because the qset was either removed or
			   reactivated. */
			update |= WHC_UPDATE_UPDATED;
			goto done;
		}

		/* Mmm, a completed qTD. */
		process_inactive_qtd(whc, qset, td);
	}

	if (!qset->remove)
		update |= qset_add_qtds(whc, qset);

done:
	/*
	 * Remove this qset from the ASL if requested, but only if has
	 * no qTDs.
	 */
	if (qset->remove && qset->ntds == 0) {
		asl_qset_remove(whc, qset);
		update |= WHC_UPDATE_REMOVED;
	}
	return update;
}

void asl_start(struct whc *whc)
{
	struct whc_qset *qset;

	qset = list_first_entry(&whc->async_list, struct whc_qset, list_node);

	le_writeq(qset->qset_dma | QH_LINK_NTDS(8), whc->base + WUSBASYNCLISTADDR);

	whc_write_wusbcmd(whc, WUSBCMD_ASYNC_EN, WUSBCMD_ASYNC_EN);
	whci_wait_for(&whc->umc->dev, whc->base + WUSBSTS,
		      WUSBSTS_ASYNC_SCHED, WUSBSTS_ASYNC_SCHED,
		      1000, "start ASL");
}

void asl_stop(struct whc *whc)
{
	whc_write_wusbcmd(whc, WUSBCMD_ASYNC_EN, 0);
	whci_wait_for(&whc->umc->dev, whc->base + WUSBSTS,
		      WUSBSTS_ASYNC_SCHED, 0,
		      1000, "stop ASL");
}

void asl_update(struct whc *whc, uint32_t wusbcmd)
{
	struct wusbhc *wusbhc = &whc->wusbhc;
	long t;

	mutex_lock(&wusbhc->mutex);
	if (wusbhc->active) {
		whc_write_wusbcmd(whc, wusbcmd, wusbcmd);
		t = wait_event_timeout(
			whc->async_list_wq,
			(le_readl(whc->base + WUSBCMD) & WUSBCMD_ASYNC_UPDATED) == 0,
			msecs_to_jiffies(1000));
		if (t == 0)
			whc_hw_error(whc, "ASL update timeout");
	}
	mutex_unlock(&wusbhc->mutex);
}

void scan_async_work(struct work_struct *work)
{
	struct whc *whc = container_of(work, struct whc, async_work);
	struct whc_qset *qset, *t;
	enum whc_update update = 0;

	spin_lock_irq(&whc->lock);

	/*
	 * Transerve the software list backwards so new qsets can be
	 * safely inserted into the ASL without making it non-circular.
	 */
	list_for_each_entry_safe_reverse(qset, t, &whc->async_list, list_node) {
		if (!qset->in_hw_list) {
			asl_qset_insert(whc, qset);
			update |= WHC_UPDATE_ADDED;
		}

		update |= process_qset(whc, qset);
	}

	spin_unlock_irq(&whc->lock);

	if (update) {
		uint32_t wusbcmd = WUSBCMD_ASYNC_UPDATED | WUSBCMD_ASYNC_SYNCED_DB;
		if (update & WHC_UPDATE_REMOVED)
			wusbcmd |= WUSBCMD_ASYNC_QSET_RM;
		asl_update(whc, wusbcmd);
	}

	/*
	 * Now that the ASL is updated, complete the removal of any
	 * removed qsets.
	 *
	 * If the qset was to be reset, do so and reinsert it into the
	 * ASL if it has pending transfers.
	 */
	spin_lock_irq(&whc->lock);

	list_for_each_entry_safe(qset, t, &whc->async_removed_list, list_node) {
		qset_remove_complete(whc, qset);
		if (qset->reset) {
			qset_reset(whc, qset);
			if (!list_empty(&qset->stds)) {
				asl_qset_insert_begin(whc, qset);
				queue_work(whc->workqueue, &whc->async_work);
			}
		}
	}

	spin_unlock_irq(&whc->lock);
}

int asl_urb_enqueue(struct whc *whc, struct urb *urb, gfp_t mem_flags)
{
	struct whc_qset *qset;
	int err;
	unsigned long flags;

	spin_lock_irqsave(&whc->lock, flags);

	err = usb_hcd_link_urb_to_ep(&whc->wusbhc.usb_hcd, urb);
	if (err < 0) {
		spin_unlock_irqrestore(&whc->lock, flags);
		return err;
	}

	qset = get_qset(whc, urb, GFP_ATOMIC);
	if (qset == NULL)
		err = -ENOMEM;
	else
		err = qset_add_urb(whc, qset, urb, GFP_ATOMIC);
	if (!err) {
		if (!qset->in_sw_list && !qset->remove)
			asl_qset_insert_begin(whc, qset);
	} else
		usb_hcd_unlink_urb_from_ep(&whc->wusbhc.usb_hcd, urb);

	spin_unlock_irqrestore(&whc->lock, flags);

	if (!err)
		queue_work(whc->workqueue, &whc->async_work);

	return err;
}

int asl_urb_dequeue(struct whc *whc, struct urb *urb, int status)
{
	struct whc_urb *wurb = urb->hcpriv;
	struct whc_qset *qset = wurb->qset;
	struct whc_std *std, *t;
	bool has_qtd = false;
	int ret;
	unsigned long flags;

	spin_lock_irqsave(&whc->lock, flags);

	ret = usb_hcd_check_unlink_urb(&whc->wusbhc.usb_hcd, urb, status);
	if (ret < 0)
		goto out;

	list_for_each_entry_safe(std, t, &qset->stds, list_node) {
		if (std->urb == urb) {
			if (std->qtd)
				has_qtd = true;
			qset_free_std(whc, std);
		} else
			std->qtd = NULL; /* so this std is re-added when the qset is */
	}

	if (has_qtd) {
		asl_qset_remove(whc, qset);
		wurb->status = status;
		wurb->is_async = true;
		queue_work(whc->workqueue, &wurb->dequeue_work);
	} else
		qset_remove_urb(whc, qset, urb, status);
out:
	spin_unlock_irqrestore(&whc->lock, flags);

	return ret;
}

void asl_qset_delete(struct whc *whc, struct whc_qset *qset)
{
	qset->remove = 1;
	queue_work(whc->workqueue, &whc->async_work);
	qset_delete(whc, qset);
}

int asl_init(struct whc *whc)
{
	struct whc_qset *qset;

	qset = qset_alloc(whc, GFP_KERNEL);
	if (qset == NULL)
		return -ENOMEM;

	asl_qset_insert_begin(whc, qset);
	asl_qset_insert(whc, qset);

	return 0;
}

void asl_clean_up(struct whc *whc)
{
	struct whc_qset *qset;

	if (!list_empty(&whc->async_list)) {
		qset = list_first_entry(&whc->async_list, struct whc_qset, list_node);
		list_del(&qset->list_node);
		qset_free(whc, qset);
	}
}
