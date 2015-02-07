

#ifndef IRCOMM_EVENT_H
#define IRCOMM_EVENT_H

#include <net/irda/irmod.h>

typedef enum {
        IRCOMM_IDLE,
        IRCOMM_WAITI,
        IRCOMM_WAITR,
        IRCOMM_CONN,
} IRCOMM_STATE;

/* IrCOMM Events */
typedef enum {
        IRCOMM_CONNECT_REQUEST,
        IRCOMM_CONNECT_RESPONSE,
        IRCOMM_TTP_CONNECT_INDICATION,
	IRCOMM_LMP_CONNECT_INDICATION,
        IRCOMM_TTP_CONNECT_CONFIRM,
	IRCOMM_LMP_CONNECT_CONFIRM,

        IRCOMM_LMP_DISCONNECT_INDICATION,
	IRCOMM_TTP_DISCONNECT_INDICATION,
        IRCOMM_DISCONNECT_REQUEST,

        IRCOMM_TTP_DATA_INDICATION,
	IRCOMM_LMP_DATA_INDICATION,
        IRCOMM_DATA_REQUEST,
        IRCOMM_CONTROL_REQUEST,
        IRCOMM_CONTROL_INDICATION,
} IRCOMM_EVENT;

struct ircomm_info {
        __u32     saddr;               /* Source device address */
        __u32     daddr;               /* Destination device address */
        __u8      dlsap_sel;
        LM_REASON reason;              /* Reason for disconnect */
	__u32     max_data_size;
	__u32     max_header_size;

	struct qos_info *qos;
};

extern const char *const ircomm_state[];

struct ircomm_cb;   /* Forward decl. */

int ircomm_do_event(struct ircomm_cb *self, IRCOMM_EVENT event,
		    struct sk_buff *skb, struct ircomm_info *info);
void ircomm_next_state(struct ircomm_cb *self, IRCOMM_STATE state);

#endif
