

#ifndef _LINUX_IF_ARCNET_H
#define _LINUX_IF_ARCNET_H

#include <linux/types.h>
#include <linux/if_ether.h>



/* CAP mode */
/* No macro but uses 1-8 */

/* RFC1201 Protocol ID's */
#define ARC_P_IP		212	/* 0xD4 */
#define ARC_P_IPV6		196	/* 0xC4: RFC2497 */
#define ARC_P_ARP		213	/* 0xD5 */
#define ARC_P_RARP		214	/* 0xD6 */
#define ARC_P_IPX		250	/* 0xFA */
#define ARC_P_NOVELL_EC		236	/* 0xEC */

/* Old RFC1051 Protocol ID's */
#define ARC_P_IP_RFC1051	240	/* 0xF0 */
#define ARC_P_ARP_RFC1051	241	/* 0xF1 */

/* MS LanMan/WfWg "NDIS" encapsulation */
#define ARC_P_ETHER		232	/* 0xE8 */

/* Unsupported/indirectly supported protocols */
#define ARC_P_DATAPOINT_BOOT	0	/* very old Datapoint equipment */
#define ARC_P_DATAPOINT_MOUNT	1
#define ARC_P_POWERLAN_BEACON	8	/* Probably ATA-Netbios related */
#define ARC_P_POWERLAN_BEACON2	243	/* 0xF3 */
#define ARC_P_LANSOFT		251	/* 0xFB - what is this? */
#define ARC_P_ATALK		0xDD

/* Hardware address length */
#define ARCNET_ALEN	1

struct arc_rfc1201 {
    __u8  proto;		/* protocol ID field - varies		*/
    __u8  split_flag;	/* for use with split packets		*/
    __be16   sequence;		/* sequence number			*/
    __u8  payload[0];	/* space remaining in packet (504 bytes)*/
};
#define RFC1201_HDR_SIZE 4


struct arc_rfc1051 {
    __u8 proto;		/* ARC_P_RFC1051_ARP/RFC1051_IP	*/
    __u8 payload[0];		/* 507 bytes			*/
};
#define RFC1051_HDR_SIZE 1


struct arc_eth_encap {
    __u8 proto;		/* Always ARC_P_ETHER			*/
    struct ethhdr eth;		/* standard ethernet header (yuck!)	*/
    __u8 payload[0];		/* 493 bytes				*/
};
#define ETH_ENCAP_HDR_SIZE 14


struct arc_cap {
	__u8 proto;
	__u8 cookie[sizeof(int)];   /* Actually NOT sent over the network */
	union {
		__u8 ack;
		__u8 raw[0];		/* 507 bytes */
	} mes;
};

struct arc_hardware {
    __u8  source,		/* source ARCnet - filled in automagically */
             dest,		/* destination ARCnet - 0 for broadcast    */
    	     offset[2];		/* offset bytes (some weird semantics)     */
};
#define ARC_HDR_SIZE 4

struct archdr {
    /* hardware requirements */
    struct arc_hardware hard;
     
    /* arcnet encapsulation-specific bits */
    union {
	struct arc_rfc1201   rfc1201;
	struct arc_rfc1051   rfc1051;
	struct arc_eth_encap eth_encap;
	struct arc_cap       cap;
	__u8 raw[0];		/* 508 bytes				*/
    } soft;
};

#endif				/* _LINUX_IF_ARCNET_H */
