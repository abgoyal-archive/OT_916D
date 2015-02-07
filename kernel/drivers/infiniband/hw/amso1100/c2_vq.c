
#include <linux/slab.h>
#include <linux/spinlock.h>

#include "c2_vq.h"
#include "c2_provider.h"


int vq_init(struct c2_dev *c2dev)
{
	sprintf(c2dev->vq_cache_name, "c2-vq:dev%c",
		(char) ('0' + c2dev->devnum));
	c2dev->host_msg_cache =
	    kmem_cache_create(c2dev->vq_cache_name, c2dev->rep_vq.msg_size, 0,
			      SLAB_HWCACHE_ALIGN, NULL);
	if (c2dev->host_msg_cache == NULL) {
		return -ENOMEM;
	}
	return 0;
}

void vq_term(struct c2_dev *c2dev)
{
	kmem_cache_destroy(c2dev->host_msg_cache);
}

struct c2_vq_req *vq_req_alloc(struct c2_dev *c2dev)
{
	struct c2_vq_req *r;

	r = kmalloc(sizeof(struct c2_vq_req), GFP_KERNEL);
	if (r) {
		init_waitqueue_head(&r->wait_object);
		r->reply_msg = (u64) NULL;
		r->event = 0;
		r->cm_id = NULL;
		r->qp = NULL;
		atomic_set(&r->refcnt, 1);
		atomic_set(&r->reply_ready, 0);
	}
	return r;
}


void vq_req_free(struct c2_dev *c2dev, struct c2_vq_req *r)
{
	r->reply_msg = (u64) NULL;
	if (atomic_dec_and_test(&r->refcnt)) {
		kfree(r);
	}
}

void vq_req_get(struct c2_dev *c2dev, struct c2_vq_req *r)
{
	atomic_inc(&r->refcnt);
}


void vq_req_put(struct c2_dev *c2dev, struct c2_vq_req *r)
{
	if (atomic_dec_and_test(&r->refcnt)) {
		if (r->reply_msg != (u64) NULL)
			vq_repbuf_free(c2dev,
				       (void *) (unsigned long) r->reply_msg);
		kfree(r);
	}
}


void *vq_repbuf_alloc(struct c2_dev *c2dev)
{
	return kmem_cache_alloc(c2dev->host_msg_cache, GFP_ATOMIC);
}

int vq_send_wr(struct c2_dev *c2dev, union c2wr *wr)
{
	void *msg;
	wait_queue_t __wait;

	/*
	 * grab adapter vq lock
	 */
	spin_lock(&c2dev->vqlock);

	/*
	 * allocate msg
	 */
	msg = c2_mq_alloc(&c2dev->req_vq);

	/*
	 * If we cannot get a msg, then we'll wait
	 * When a messages are available, the int handler will wake_up()
	 * any waiters.
	 */
	while (msg == NULL) {
		pr_debug("%s:%d no available msg in VQ, waiting...\n",
		       __func__, __LINE__);
		init_waitqueue_entry(&__wait, current);
		add_wait_queue(&c2dev->req_vq_wo, &__wait);
		spin_unlock(&c2dev->vqlock);
		for (;;) {
			set_current_state(TASK_INTERRUPTIBLE);
			if (!c2_mq_full(&c2dev->req_vq)) {
				break;
			}
			if (!signal_pending(current)) {
				schedule_timeout(1 * HZ);	/* 1 second... */
				continue;
			}
			set_current_state(TASK_RUNNING);
			remove_wait_queue(&c2dev->req_vq_wo, &__wait);
			return -EINTR;
		}
		set_current_state(TASK_RUNNING);
		remove_wait_queue(&c2dev->req_vq_wo, &__wait);
		spin_lock(&c2dev->vqlock);
		msg = c2_mq_alloc(&c2dev->req_vq);
	}

	/*
	 * copy wr into adapter msg
	 */
	memcpy(msg, wr, c2dev->req_vq.msg_size);

	/*
	 * post msg
	 */
	c2_mq_produce(&c2dev->req_vq);

	/*
	 * release adapter vq lock
	 */
	spin_unlock(&c2dev->vqlock);
	return 0;
}


int vq_wait_for_reply(struct c2_dev *c2dev, struct c2_vq_req *req)
{
	if (!wait_event_timeout(req->wait_object,
				atomic_read(&req->reply_ready),
				60*HZ))
		return -ETIMEDOUT;

	return 0;
}

void vq_repbuf_free(struct c2_dev *c2dev, void *reply)
{
	kmem_cache_free(c2dev->host_msg_cache, reply);
}
