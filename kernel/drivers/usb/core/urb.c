
#include <linux/module.h>
#include <linux/string.h>
#include <linux/bitops.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/log2.h>
#include <linux/usb.h>
#include <linux/wait.h>
#include <linux/usb/hcd.h>

#define to_urb(d) container_of(d, struct urb, kref)


static void urb_destroy(struct kref *kref)
{
	struct urb *urb = to_urb(kref);

	if (urb->transfer_flags & URB_FREE_BUFFER)
		kfree(urb->transfer_buffer);

	kfree(urb);
}

void usb_init_urb(struct urb *urb)
{
	if (urb) {
		memset(urb, 0, sizeof(*urb));
		kref_init(&urb->kref);
		INIT_LIST_HEAD(&urb->anchor_list);
	}
}
EXPORT_SYMBOL_GPL(usb_init_urb);

struct urb *usb_alloc_urb(int iso_packets, gfp_t mem_flags)
{
	struct urb *urb;

	urb = kmalloc(sizeof(struct urb) +
		iso_packets * sizeof(struct usb_iso_packet_descriptor),
		mem_flags);
	if (!urb) {
		printk(KERN_ERR "alloc_urb: kmalloc failed\n");
		return NULL;
	}
	usb_init_urb(urb);
	return urb;
}
EXPORT_SYMBOL_GPL(usb_alloc_urb);

void usb_free_urb(struct urb *urb)
{
	if (urb)
		kref_put(&urb->kref, urb_destroy);
}
EXPORT_SYMBOL_GPL(usb_free_urb);

struct urb *usb_get_urb(struct urb *urb)
{
	if (urb)
		kref_get(&urb->kref);
	return urb;
}
EXPORT_SYMBOL_GPL(usb_get_urb);

void usb_anchor_urb(struct urb *urb, struct usb_anchor *anchor)
{
	unsigned long flags;

	spin_lock_irqsave(&anchor->lock, flags);
	usb_get_urb(urb);
	list_add_tail(&urb->anchor_list, &anchor->urb_list);
	urb->anchor = anchor;

	if (unlikely(anchor->poisoned)) {
		atomic_inc(&urb->reject);
	}

	spin_unlock_irqrestore(&anchor->lock, flags);
}
EXPORT_SYMBOL_GPL(usb_anchor_urb);

/* Callers must hold anchor->lock */
static void __usb_unanchor_urb(struct urb *urb, struct usb_anchor *anchor)
{
	urb->anchor = NULL;
	list_del(&urb->anchor_list);
	usb_put_urb(urb);
	if (list_empty(&anchor->urb_list))
		wake_up(&anchor->wait);
}

void usb_unanchor_urb(struct urb *urb)
{
	unsigned long flags;
	struct usb_anchor *anchor;

	if (!urb)
		return;

	anchor = urb->anchor;
	if (!anchor)
		return;

	spin_lock_irqsave(&anchor->lock, flags);
	/*
	 * At this point, we could be competing with another thread which
	 * has the same intention. To protect the urb from being unanchored
	 * twice, only the winner of the race gets the job.
	 */
	if (likely(anchor == urb->anchor))
		__usb_unanchor_urb(urb, anchor);
	spin_unlock_irqrestore(&anchor->lock, flags);
}
EXPORT_SYMBOL_GPL(usb_unanchor_urb);

/*-------------------------------------------------------------------*/

int usb_submit_urb(struct urb *urb, gfp_t mem_flags)
{
	int				xfertype, max;
	struct usb_device		*dev;
	struct usb_host_endpoint	*ep;
	int				is_out;

	if (!urb || urb->hcpriv || !urb->complete)
		return -EINVAL;
	dev = urb->dev;
	if ((!dev) || (dev->state < USB_STATE_UNAUTHENTICATED))
		return -ENODEV;

	/* For now, get the endpoint from the pipe.  Eventually drivers
	 * will be required to set urb->ep directly and we will eliminate
	 * urb->pipe.
	 */
	ep = usb_pipe_endpoint(dev, urb->pipe);
	if (!ep)
		return -ENOENT;

	urb->ep = ep;
	urb->status = -EINPROGRESS;
	urb->actual_length = 0;

	/* Lots of sanity checks, so HCDs can rely on clean data
	 * and don't need to duplicate tests
	 */
	xfertype = usb_endpoint_type(&ep->desc);
	if (xfertype == USB_ENDPOINT_XFER_CONTROL) {
		struct usb_ctrlrequest *setup =
				(struct usb_ctrlrequest *) urb->setup_packet;

		if (!setup)
			return -ENOEXEC;
		is_out = !(setup->bRequestType & USB_DIR_IN) ||
				!setup->wLength;
	} else {
		is_out = usb_endpoint_dir_out(&ep->desc);
	}

	/* Clear the internal flags and cache the direction for later use */
	urb->transfer_flags &= ~(URB_DIR_MASK | URB_DMA_MAP_SINGLE |
			URB_DMA_MAP_PAGE | URB_DMA_MAP_SG | URB_MAP_LOCAL |
			URB_SETUP_MAP_SINGLE | URB_SETUP_MAP_LOCAL |
			URB_DMA_SG_COMBINED);
	urb->transfer_flags |= (is_out ? URB_DIR_OUT : URB_DIR_IN);

	if (xfertype != USB_ENDPOINT_XFER_CONTROL &&
			dev->state < USB_STATE_CONFIGURED)
		return -ENODEV;

	max = le16_to_cpu(ep->desc.wMaxPacketSize);
	if (max <= 0) {
		dev_dbg(&dev->dev,
			"bogus endpoint ep%d%s in %s (bad maxpacket %d)\n",
			usb_endpoint_num(&ep->desc), is_out ? "out" : "in",
			__func__, max);
		return -EMSGSIZE;
	}

	/* periodic transfers limit size per frame/uframe,
	 * but drivers only control those sizes for ISO.
	 * while we're checking, initialize return status.
	 */
	if (xfertype == USB_ENDPOINT_XFER_ISOC) {
		int	n, len;

		/* FIXME SuperSpeed isoc endpoints have up to 16 bursts */
		/* "high bandwidth" mode, 1-3 packets/uframe? */
		if (dev->speed == USB_SPEED_HIGH) {
			int	mult = 1 + ((max >> 11) & 0x03);
			max &= 0x07ff;
			max *= mult;
		}

		if (urb->number_of_packets <= 0)
			return -EINVAL;
		for (n = 0; n < urb->number_of_packets; n++) {
			len = urb->iso_frame_desc[n].length;
			if (len < 0 || len > max)
				return -EMSGSIZE;
			urb->iso_frame_desc[n].status = -EXDEV;
			urb->iso_frame_desc[n].actual_length = 0;
		}
	}

	/* the I/O buffer must be mapped/unmapped, except when length=0 */
	if (urb->transfer_buffer_length > INT_MAX)
		return -EMSGSIZE;

#ifdef DEBUG
	/* stuff that drivers shouldn't do, but which shouldn't
	 * cause problems in HCDs if they get it wrong.
	 */
	{
	unsigned int	orig_flags = urb->transfer_flags;
	unsigned int	allowed;
	static int pipetypes[4] = {
		PIPE_CONTROL, PIPE_ISOCHRONOUS, PIPE_BULK, PIPE_INTERRUPT
	};

	/* Check that the pipe's type matches the endpoint's type */
	if (usb_pipetype(urb->pipe) != pipetypes[xfertype])
		return -EPIPE;		/* The most suitable error code :-) */

	/* enforce simple/standard policy */
	allowed = (URB_NO_TRANSFER_DMA_MAP | URB_NO_INTERRUPT | URB_DIR_MASK |
			URB_FREE_BUFFER);
	switch (xfertype) {
	case USB_ENDPOINT_XFER_BULK:
		if (is_out)
			allowed |= URB_ZERO_PACKET;
		/* FALLTHROUGH */
	case USB_ENDPOINT_XFER_CONTROL:
		allowed |= URB_NO_FSBR;	/* only affects UHCI */
		/* FALLTHROUGH */
	default:			/* all non-iso endpoints */
		if (!is_out)
			allowed |= URB_SHORT_NOT_OK;
		break;
	case USB_ENDPOINT_XFER_ISOC:
		allowed |= URB_ISO_ASAP;
		break;
	}
	urb->transfer_flags &= allowed;

	/* fail if submitter gave bogus flags */
	if (urb->transfer_flags != orig_flags) {
		dev_err(&dev->dev, "BOGUS urb flags, %x --> %x\n",
			orig_flags, urb->transfer_flags);
		return -EINVAL;
	}
	}
#endif
	/*
	 * Force periodic transfer intervals to be legal values that are
	 * a power of two (so HCDs don't need to).
	 *
	 * FIXME want bus->{intr,iso}_sched_horizon values here.  Each HC
	 * supports different values... this uses EHCI/UHCI defaults (and
	 * EHCI can use smaller non-default values).
	 */
	switch (xfertype) {
	case USB_ENDPOINT_XFER_ISOC:
	case USB_ENDPOINT_XFER_INT:
		/* too small? */
		switch (dev->speed) {
		case USB_SPEED_WIRELESS:
			if (urb->interval < 6)
				return -EINVAL;
			break;
		default:
			if (urb->interval <= 0)
				return -EINVAL;
			break;
		}
		/* too big? */
		switch (dev->speed) {
		case USB_SPEED_SUPER:	/* units are 125us */
			/* Handle up to 2^(16-1) microframes */
			if (urb->interval > (1 << 15))
				return -EINVAL;
			max = 1 << 15;
			break;
		case USB_SPEED_WIRELESS:
			if (urb->interval > 16)
				return -EINVAL;
			break;
		case USB_SPEED_HIGH:	/* units are microframes */
			/* NOTE usb handles 2^15 */
			if (urb->interval > (1024 * 8))
				urb->interval = 1024 * 8;
			max = 1024 * 8;
			break;
		case USB_SPEED_FULL:	/* units are frames/msec */
		case USB_SPEED_LOW:
			if (xfertype == USB_ENDPOINT_XFER_INT) {
				if (urb->interval > 255)
					return -EINVAL;
				/* NOTE ohci only handles up to 32 */
				max = 128;
			} else {
				if (urb->interval > 1024)
					urb->interval = 1024;
				/* NOTE usb and ohci handle up to 2^15 */
				max = 1024;
			}
			break;
		default:
			return -EINVAL;
		}
		if (dev->speed != USB_SPEED_WIRELESS) {
			/* Round down to a power of 2, no more than max */
			urb->interval = min(max, 1 << ilog2(urb->interval));
		}
	}

	return usb_hcd_submit_urb(urb, mem_flags);
}
EXPORT_SYMBOL_GPL(usb_submit_urb);

/*-------------------------------------------------------------------*/

int usb_unlink_urb(struct urb *urb)
{
	if (!urb)
		return -EINVAL;
	if (!urb->dev)
		return -ENODEV;
	if (!urb->ep)
		return -EIDRM;
	return usb_hcd_unlink_urb(urb, -ECONNRESET);
}
EXPORT_SYMBOL_GPL(usb_unlink_urb);

void usb_kill_urb(struct urb *urb)
{
	might_sleep();
	if (!(urb && urb->dev && urb->ep))
		return;
	atomic_inc(&urb->reject);

	usb_hcd_unlink_urb(urb, -ENOENT);
	wait_event(usb_kill_urb_queue, atomic_read(&urb->use_count) == 0);

	atomic_dec(&urb->reject);
}
EXPORT_SYMBOL_GPL(usb_kill_urb);

void usb_poison_urb(struct urb *urb)
{
	might_sleep();
	if (!(urb && urb->dev && urb->ep))
		return;
	atomic_inc(&urb->reject);

	usb_hcd_unlink_urb(urb, -ENOENT);
	wait_event(usb_kill_urb_queue, atomic_read(&urb->use_count) == 0);
}
EXPORT_SYMBOL_GPL(usb_poison_urb);

void usb_unpoison_urb(struct urb *urb)
{
	if (!urb)
		return;

	atomic_dec(&urb->reject);
}
EXPORT_SYMBOL_GPL(usb_unpoison_urb);

void usb_kill_anchored_urbs(struct usb_anchor *anchor)
{
	struct urb *victim;

	spin_lock_irq(&anchor->lock);
	while (!list_empty(&anchor->urb_list)) {
		victim = list_entry(anchor->urb_list.prev, struct urb,
				    anchor_list);
		/* we must make sure the URB isn't freed before we kill it*/
		usb_get_urb(victim);
		spin_unlock_irq(&anchor->lock);
		/* this will unanchor the URB */
		usb_kill_urb(victim);
		usb_put_urb(victim);
		spin_lock_irq(&anchor->lock);
	}
	spin_unlock_irq(&anchor->lock);
}
EXPORT_SYMBOL_GPL(usb_kill_anchored_urbs);


void usb_poison_anchored_urbs(struct usb_anchor *anchor)
{
	struct urb *victim;

	spin_lock_irq(&anchor->lock);
	anchor->poisoned = 1;
	while (!list_empty(&anchor->urb_list)) {
		victim = list_entry(anchor->urb_list.prev, struct urb,
				    anchor_list);
		/* we must make sure the URB isn't freed before we kill it*/
		usb_get_urb(victim);
		spin_unlock_irq(&anchor->lock);
		/* this will unanchor the URB */
		usb_poison_urb(victim);
		usb_put_urb(victim);
		spin_lock_irq(&anchor->lock);
	}
	spin_unlock_irq(&anchor->lock);
}
EXPORT_SYMBOL_GPL(usb_poison_anchored_urbs);

void usb_unpoison_anchored_urbs(struct usb_anchor *anchor)
{
	unsigned long flags;
	struct urb *lazarus;

	spin_lock_irqsave(&anchor->lock, flags);
	list_for_each_entry(lazarus, &anchor->urb_list, anchor_list) {
		usb_unpoison_urb(lazarus);
	}
	anchor->poisoned = 0;
	spin_unlock_irqrestore(&anchor->lock, flags);
}
EXPORT_SYMBOL_GPL(usb_unpoison_anchored_urbs);
void usb_unlink_anchored_urbs(struct usb_anchor *anchor)
{
	struct urb *victim;

	while ((victim = usb_get_from_anchor(anchor)) != NULL) {
		usb_unlink_urb(victim);
		usb_put_urb(victim);
	}
}
EXPORT_SYMBOL_GPL(usb_unlink_anchored_urbs);

int usb_wait_anchor_empty_timeout(struct usb_anchor *anchor,
				  unsigned int timeout)
{
	return wait_event_timeout(anchor->wait, list_empty(&anchor->urb_list),
				  msecs_to_jiffies(timeout));
}
EXPORT_SYMBOL_GPL(usb_wait_anchor_empty_timeout);

struct urb *usb_get_from_anchor(struct usb_anchor *anchor)
{
	struct urb *victim;
	unsigned long flags;

	spin_lock_irqsave(&anchor->lock, flags);
	if (!list_empty(&anchor->urb_list)) {
		victim = list_entry(anchor->urb_list.next, struct urb,
				    anchor_list);
		usb_get_urb(victim);
		__usb_unanchor_urb(victim, anchor);
	} else {
		victim = NULL;
	}
	spin_unlock_irqrestore(&anchor->lock, flags);

	return victim;
}

EXPORT_SYMBOL_GPL(usb_get_from_anchor);

void usb_scuttle_anchored_urbs(struct usb_anchor *anchor)
{
	struct urb *victim;
	unsigned long flags;

	spin_lock_irqsave(&anchor->lock, flags);
	while (!list_empty(&anchor->urb_list)) {
		victim = list_entry(anchor->urb_list.prev, struct urb,
				    anchor_list);
		__usb_unanchor_urb(victim, anchor);
	}
	spin_unlock_irqrestore(&anchor->lock, flags);
}

EXPORT_SYMBOL_GPL(usb_scuttle_anchored_urbs);

int usb_anchor_empty(struct usb_anchor *anchor)
{
	return list_empty(&anchor->urb_list);
}

EXPORT_SYMBOL_GPL(usb_anchor_empty);

