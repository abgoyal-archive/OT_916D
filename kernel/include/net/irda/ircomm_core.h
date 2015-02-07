

#ifndef IRCOMM_CORE_H
#define IRCOMM_CORE_H

#include <net/irda/irda.h>
#include <net/irda/irqueue.h>
#include <net/irda/ircomm_event.h>

#define IRCOMM_MAGIC 0x98347298
#define IRCOMM_HEADER_SIZE 1

struct ircomm_cb;   /* Forward decl. */

typedef struct {
	int (*data_request)(struct ircomm_cb *, struct sk_buff *, int clen);
	int (*connect_request)(struct ircomm_cb *, struct sk_buff *, 
			       struct ircomm_info *);
	int (*connect_response)(struct ircomm_cb *, struct sk_buff *);
	int (*disconnect_request)(struct ircomm_cb *, struct sk_buff *, 
				  struct ircomm_info *);	
} call_t;

struct ircomm_cb {
	irda_queue_t queue;
	magic_t magic;

	notify_t notify;
	call_t   issue;

	int state;
	int line;            /* Which TTY line we are using */

	struct tsap_cb *tsap;
	struct lsap_cb *lsap;
	
	__u8 dlsap_sel;      /* Destination LSAP/TSAP selector */
	__u8 slsap_sel;      /* Source LSAP/TSAP selector */

	__u32 saddr;         /* Source device address (link we are using) */
	__u32 daddr;         /* Destination device address */

	int max_header_size; /* Header space we must reserve for each frame */
	int max_data_size;   /* The amount of data we can fill in each frame */

	LOCAL_FLOW flow_status; /* Used by ircomm_lmp */
	int pkt_count;          /* Number of frames we have sent to IrLAP */

	__u8 service_type;
};

extern hashbin_t *ircomm;

struct ircomm_cb *ircomm_open(notify_t *notify, __u8 service_type, int line);
int ircomm_close(struct ircomm_cb *self);

int ircomm_data_request(struct ircomm_cb *self, struct sk_buff *skb);
void ircomm_data_indication(struct ircomm_cb *self, struct sk_buff *skb);
void ircomm_process_data(struct ircomm_cb *self, struct sk_buff *skb);
int ircomm_control_request(struct ircomm_cb *self, struct sk_buff *skb);
int ircomm_connect_request(struct ircomm_cb *self, __u8 dlsap_sel, 
			   __u32 saddr, __u32 daddr, struct sk_buff *skb,
			   __u8 service_type);
void ircomm_connect_indication(struct ircomm_cb *self, struct sk_buff *skb,
			       struct ircomm_info *info);
void ircomm_connect_confirm(struct ircomm_cb *self, struct sk_buff *skb,
			    struct ircomm_info *info);
int ircomm_connect_response(struct ircomm_cb *self, struct sk_buff *userdata);
int ircomm_disconnect_request(struct ircomm_cb *self, struct sk_buff *userdata);
void ircomm_disconnect_indication(struct ircomm_cb *self, struct sk_buff *skb,
				  struct ircomm_info *info);
void ircomm_flow_request(struct ircomm_cb *self, LOCAL_FLOW flow);

#define ircomm_is_connected(self) (self->state == IRCOMM_CONN)

#endif
