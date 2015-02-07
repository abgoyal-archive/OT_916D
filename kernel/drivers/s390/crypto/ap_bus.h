

#ifndef _AP_BUS_H_
#define _AP_BUS_H_

#include <linux/device.h>
#include <linux/mod_devicetable.h>
#include <linux/types.h>

#define AP_DEVICES 64		/* Number of AP devices. */
#define AP_DOMAINS 16		/* Number of AP domains. */
#define AP_MAX_RESET 90		/* Maximum number of resets. */
#define AP_RESET_TIMEOUT (HZ/2)	/* Time in ticks for reset timeouts. */
#define AP_CONFIG_TIME 30	/* Time in seconds between AP bus rescans. */
#define AP_POLL_TIME 1		/* Time in ticks between receive polls. */

extern int ap_domain_index;

typedef unsigned int ap_qid_t;

#define AP_MKQID(_device,_queue) (((_device) & 63) << 8 | ((_queue) & 15))
#define AP_QID_DEVICE(_qid) (((_qid) >> 8) & 63)
#define AP_QID_QUEUE(_qid) ((_qid) & 15)

struct ap_queue_status {
	unsigned int queue_empty	: 1;
	unsigned int replies_waiting	: 1;
	unsigned int queue_full		: 1;
	unsigned int pad1		: 4;
	unsigned int int_enabled	: 1;
	unsigned int response_code	: 8;
	unsigned int pad2		: 16;
};

#define AP_RESPONSE_NORMAL		0x00
#define AP_RESPONSE_Q_NOT_AVAIL		0x01
#define AP_RESPONSE_RESET_IN_PROGRESS	0x02
#define AP_RESPONSE_DECONFIGURED	0x03
#define AP_RESPONSE_CHECKSTOPPED	0x04
#define AP_RESPONSE_BUSY		0x05
#define AP_RESPONSE_INVALID_ADDRESS	0x06
#define AP_RESPONSE_OTHERWISE_CHANGED	0x07
#define AP_RESPONSE_Q_FULL		0x10
#define AP_RESPONSE_NO_PENDING_REPLY	0x10
#define AP_RESPONSE_INDEX_TOO_BIG	0x11
#define AP_RESPONSE_NO_FIRST_PART	0x13
#define AP_RESPONSE_MESSAGE_TOO_BIG	0x15
#define AP_RESPONSE_REQ_FAC_NOT_INST	0x16

#define AP_DEVICE_TYPE_PCICC	3
#define AP_DEVICE_TYPE_PCICA	4
#define AP_DEVICE_TYPE_PCIXCC	5
#define AP_DEVICE_TYPE_CEX2A	6
#define AP_DEVICE_TYPE_CEX2C	7
#define AP_DEVICE_TYPE_CEX3A	8
#define AP_DEVICE_TYPE_CEX3C	9

#define AP_RESET_IGNORE	0	/* request timeout will be ignored */
#define AP_RESET_ARMED	1	/* request timeout timer is active */
#define AP_RESET_DO	2	/* AP reset required */

struct ap_device;
struct ap_message;

struct ap_driver {
	struct device_driver driver;
	struct ap_device_id *ids;

	int (*probe)(struct ap_device *);
	void (*remove)(struct ap_device *);
	/* receive is called from tasklet context */
	void (*receive)(struct ap_device *, struct ap_message *,
			struct ap_message *);
	int request_timeout;		/* request timeout in jiffies */
};

#define to_ap_drv(x) container_of((x), struct ap_driver, driver)

int ap_driver_register(struct ap_driver *, struct module *, char *);
void ap_driver_unregister(struct ap_driver *);

struct ap_device {
	struct device device;
	struct ap_driver *drv;		/* Pointer to AP device driver. */
	spinlock_t lock;		/* Per device lock. */
	struct list_head list;		/* private list of all AP devices. */

	ap_qid_t qid;			/* AP queue id. */
	int queue_depth;		/* AP queue depth.*/
	int device_type;		/* AP device type. */
	int unregistered;		/* marks AP device as unregistered */
	struct timer_list timeout;	/* Timer for request timeouts. */
	int reset;			/* Reset required after req. timeout. */

	int queue_count;		/* # messages currently on AP queue. */

	struct list_head pendingq;	/* List of message sent to AP queue. */
	int pendingq_count;		/* # requests on pendingq list. */
	struct list_head requestq;	/* List of message yet to be sent. */
	int requestq_count;		/* # requests on requestq list. */
	int total_request_count;	/* # requests ever for this AP device. */

	struct ap_message *reply;	/* Per device reply message. */

	void *private;			/* ap driver private pointer. */
};

#define to_ap_dev(x) container_of((x), struct ap_device, device)

struct ap_message {
	struct list_head list;		/* Request queueing. */
	unsigned long long psmid;	/* Message id. */
	void *message;			/* Pointer to message buffer. */
	size_t length;			/* Message length. */

	void *private;			/* ap driver private pointer. */
	unsigned int special:1;		/* Used for special commands. */
};

#define AP_DEVICE(dt)					\
	.dev_type=(dt),					\
	.match_flags=AP_DEVICE_ID_MATCH_DEVICE_TYPE,

static inline void ap_init_message(struct ap_message *ap_msg)
{
	ap_msg->psmid = 0;
	ap_msg->length = 0;
	ap_msg->special = 0;
}

int ap_send(ap_qid_t, unsigned long long, void *, size_t);
int ap_recv(ap_qid_t, unsigned long long *, void *, size_t);

void ap_queue_message(struct ap_device *ap_dev, struct ap_message *ap_msg);
void ap_cancel_message(struct ap_device *ap_dev, struct ap_message *ap_msg);
void ap_flush_queue(struct ap_device *ap_dev);

int ap_module_init(void);
void ap_module_exit(void);

#endif /* _AP_BUS_H_ */
