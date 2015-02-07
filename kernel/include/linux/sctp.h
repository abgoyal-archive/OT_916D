
#ifndef __LINUX_SCTP_H__
#define __LINUX_SCTP_H__

#include <linux/in.h>		/* We need in_addr.  */
#include <linux/in6.h>		/* We need in6_addr.  */


/* Section 3.1.  SCTP Common Header Format */
typedef struct sctphdr {
	__be16 source;
	__be16 dest;
	__be32 vtag;
	__le32 checksum;
} __attribute__((packed)) sctp_sctphdr_t;

#ifdef __KERNEL__
#include <linux/skbuff.h>

static inline struct sctphdr *sctp_hdr(const struct sk_buff *skb)
{
	return (struct sctphdr *)skb_transport_header(skb);
}
#endif

/* Section 3.2.  Chunk Field Descriptions. */
typedef struct sctp_chunkhdr {
	__u8 type;
	__u8 flags;
	__be16 length;
} __attribute__((packed)) sctp_chunkhdr_t;


typedef enum {
	SCTP_CID_DATA			= 0,
        SCTP_CID_INIT			= 1,
        SCTP_CID_INIT_ACK		= 2,
        SCTP_CID_SACK			= 3,
        SCTP_CID_HEARTBEAT		= 4,
        SCTP_CID_HEARTBEAT_ACK		= 5,
        SCTP_CID_ABORT			= 6,
        SCTP_CID_SHUTDOWN		= 7,
        SCTP_CID_SHUTDOWN_ACK		= 8,
        SCTP_CID_ERROR			= 9,
        SCTP_CID_COOKIE_ECHO		= 10,
        SCTP_CID_COOKIE_ACK	        = 11,
        SCTP_CID_ECN_ECNE		= 12,
        SCTP_CID_ECN_CWR		= 13,
        SCTP_CID_SHUTDOWN_COMPLETE	= 14,

	/* AUTH Extension Section 4.1 */
	SCTP_CID_AUTH			= 0x0F,

	/* PR-SCTP Sec 3.2 */
	SCTP_CID_FWD_TSN		= 0xC0,

	/* Use hex, as defined in ADDIP sec. 3.1 */
	SCTP_CID_ASCONF			= 0xC1,
	SCTP_CID_ASCONF_ACK		= 0x80,
} sctp_cid_t; /* enum */


typedef enum {
	SCTP_CID_ACTION_DISCARD     = 0x00,
	SCTP_CID_ACTION_DISCARD_ERR = 0x40,
	SCTP_CID_ACTION_SKIP        = 0x80,
	SCTP_CID_ACTION_SKIP_ERR    = 0xc0,
} sctp_cid_action_t;

enum { SCTP_CID_ACTION_MASK = 0xc0, };

enum { SCTP_CHUNK_FLAG_T = 0x01 };


#define sctp_test_T_bit(c)    ((c)->chunk_hdr->flags & SCTP_CHUNK_FLAG_T)


typedef struct sctp_paramhdr {
	__be16 type;
	__be16 length;
} __attribute__((packed)) sctp_paramhdr_t;

typedef enum {

	/* RFC 2960 Section 3.3.5 */
	SCTP_PARAM_HEARTBEAT_INFO		= cpu_to_be16(1),
	/* RFC 2960 Section 3.3.2.1 */
	SCTP_PARAM_IPV4_ADDRESS			= cpu_to_be16(5),
	SCTP_PARAM_IPV6_ADDRESS			= cpu_to_be16(6),
	SCTP_PARAM_STATE_COOKIE			= cpu_to_be16(7),
	SCTP_PARAM_UNRECOGNIZED_PARAMETERS	= cpu_to_be16(8),
	SCTP_PARAM_COOKIE_PRESERVATIVE		= cpu_to_be16(9),
	SCTP_PARAM_HOST_NAME_ADDRESS		= cpu_to_be16(11),
	SCTP_PARAM_SUPPORTED_ADDRESS_TYPES	= cpu_to_be16(12),
	SCTP_PARAM_ECN_CAPABLE			= cpu_to_be16(0x8000),

	/* AUTH Extension Section 3 */
	SCTP_PARAM_RANDOM			= cpu_to_be16(0x8002),
	SCTP_PARAM_CHUNKS			= cpu_to_be16(0x8003),
	SCTP_PARAM_HMAC_ALGO			= cpu_to_be16(0x8004),

	/* Add-IP: Supported Extensions, Section 4.2 */
	SCTP_PARAM_SUPPORTED_EXT	= cpu_to_be16(0x8008),

	/* PR-SCTP Sec 3.1 */
	SCTP_PARAM_FWD_TSN_SUPPORT	= cpu_to_be16(0xc000),

	/* Add-IP Extension. Section 3.2 */
	SCTP_PARAM_ADD_IP		= cpu_to_be16(0xc001),
	SCTP_PARAM_DEL_IP		= cpu_to_be16(0xc002),
	SCTP_PARAM_ERR_CAUSE		= cpu_to_be16(0xc003),
	SCTP_PARAM_SET_PRIMARY		= cpu_to_be16(0xc004),
	SCTP_PARAM_SUCCESS_REPORT	= cpu_to_be16(0xc005),
	SCTP_PARAM_ADAPTATION_LAYER_IND = cpu_to_be16(0xc006),

} sctp_param_t; /* enum */


typedef enum {
	SCTP_PARAM_ACTION_DISCARD     = cpu_to_be16(0x0000),
	SCTP_PARAM_ACTION_DISCARD_ERR = cpu_to_be16(0x4000),
	SCTP_PARAM_ACTION_SKIP        = cpu_to_be16(0x8000),
	SCTP_PARAM_ACTION_SKIP_ERR    = cpu_to_be16(0xc000),
} sctp_param_action_t;

enum { SCTP_PARAM_ACTION_MASK = cpu_to_be16(0xc000), };

/* RFC 2960 Section 3.3.1 Payload Data (DATA) (0) */

typedef struct sctp_datahdr {
	__be32 tsn;
	__be16 stream;
	__be16 ssn;
	__be32 ppid;
	__u8  payload[0];
} __attribute__((packed)) sctp_datahdr_t;

typedef struct sctp_data_chunk {
        sctp_chunkhdr_t chunk_hdr;
        sctp_datahdr_t  data_hdr;
} __attribute__((packed)) sctp_data_chunk_t;

/* DATA Chuck Specific Flags */
enum {
	SCTP_DATA_MIDDLE_FRAG	= 0x00,
	SCTP_DATA_LAST_FRAG	= 0x01,
	SCTP_DATA_FIRST_FRAG	= 0x02,
	SCTP_DATA_NOT_FRAG	= 0x03,
	SCTP_DATA_UNORDERED	= 0x04,
	SCTP_DATA_SACK_IMM	= 0x08,
};
enum { SCTP_DATA_FRAG_MASK = 0x03, };


typedef struct sctp_inithdr {
	__be32 init_tag;
	__be32 a_rwnd;
	__be16 num_outbound_streams;
	__be16 num_inbound_streams;
	__be32 initial_tsn;
	__u8  params[0];
} __attribute__((packed)) sctp_inithdr_t;

typedef struct sctp_init_chunk {
	sctp_chunkhdr_t chunk_hdr;
	sctp_inithdr_t init_hdr;
} __attribute__((packed)) sctp_init_chunk_t;


/* Section 3.3.2.1. IPv4 Address Parameter (5) */
typedef struct sctp_ipv4addr_param {
	sctp_paramhdr_t param_hdr;
	struct in_addr  addr;
} __attribute__((packed)) sctp_ipv4addr_param_t;

/* Section 3.3.2.1. IPv6 Address Parameter (6) */
typedef struct sctp_ipv6addr_param {
	sctp_paramhdr_t param_hdr;
	struct in6_addr addr;
} __attribute__((packed)) sctp_ipv6addr_param_t;

/* Section 3.3.2.1 Cookie Preservative (9) */
typedef struct sctp_cookie_preserve_param {
	sctp_paramhdr_t param_hdr;
	__be32          lifespan_increment;
} __attribute__((packed)) sctp_cookie_preserve_param_t;

/* Section 3.3.2.1 Host Name Address (11) */
typedef struct sctp_hostname_param {
	sctp_paramhdr_t param_hdr;
	uint8_t hostname[0];
} __attribute__((packed)) sctp_hostname_param_t;

/* Section 3.3.2.1 Supported Address Types (12) */
typedef struct sctp_supported_addrs_param {
	sctp_paramhdr_t param_hdr;
	__be16 types[0];
} __attribute__((packed)) sctp_supported_addrs_param_t;

/* Appendix A. ECN Capable (32768) */
typedef struct sctp_ecn_capable_param {
	sctp_paramhdr_t param_hdr;
} __attribute__((packed)) sctp_ecn_capable_param_t;

/* ADDIP Section 3.2.6 Adaptation Layer Indication */
typedef struct sctp_adaptation_ind_param {
	struct sctp_paramhdr param_hdr;
	__be32 adaptation_ind;
} __attribute__((packed)) sctp_adaptation_ind_param_t;

/* ADDIP Section 4.2.7 Supported Extensions Parameter */
typedef struct sctp_supported_ext_param {
	struct sctp_paramhdr param_hdr;
	__u8 chunks[0];
} __attribute__((packed)) sctp_supported_ext_param_t;

/* AUTH Section 3.1 Random */
typedef struct sctp_random_param {
	sctp_paramhdr_t param_hdr;
	__u8 random_val[0];
} __attribute__((packed)) sctp_random_param_t;

/* AUTH Section 3.2 Chunk List */
typedef struct sctp_chunks_param {
	sctp_paramhdr_t param_hdr;
	__u8 chunks[0];
} __attribute__((packed)) sctp_chunks_param_t;

/* AUTH Section 3.3 HMAC Algorithm */
typedef struct sctp_hmac_algo_param {
	sctp_paramhdr_t param_hdr;
	__be16 hmac_ids[0];
} __attribute__((packed)) sctp_hmac_algo_param_t;

typedef sctp_init_chunk_t sctp_initack_chunk_t;

/* Section 3.3.3.1 State Cookie (7) */
typedef struct sctp_cookie_param {
	sctp_paramhdr_t p;
	__u8 body[0];
} __attribute__((packed)) sctp_cookie_param_t;

/* Section 3.3.3.1 Unrecognized Parameters (8) */
typedef struct sctp_unrecognized_param {
	sctp_paramhdr_t param_hdr;
	sctp_paramhdr_t unrecognized;
} __attribute__((packed)) sctp_unrecognized_param_t;




typedef struct sctp_gap_ack_block {
	__be16 start;
	__be16 end;
} __attribute__((packed)) sctp_gap_ack_block_t;

typedef __be32 sctp_dup_tsn_t;

typedef union {
	sctp_gap_ack_block_t	gab;
        sctp_dup_tsn_t		dup;
} sctp_sack_variable_t;

typedef struct sctp_sackhdr {
	__be32 cum_tsn_ack;
	__be32 a_rwnd;
	__be16 num_gap_ack_blocks;
	__be16 num_dup_tsns;
	sctp_sack_variable_t variable[0];
} __attribute__((packed)) sctp_sackhdr_t;

typedef struct sctp_sack_chunk {
	sctp_chunkhdr_t chunk_hdr;
	sctp_sackhdr_t sack_hdr;
} __attribute__((packed)) sctp_sack_chunk_t;



typedef struct sctp_heartbeathdr {
	sctp_paramhdr_t info;
} __attribute__((packed)) sctp_heartbeathdr_t;

typedef struct sctp_heartbeat_chunk {
	sctp_chunkhdr_t chunk_hdr;
	sctp_heartbeathdr_t hb_hdr;
} __attribute__((packed)) sctp_heartbeat_chunk_t;


typedef struct sctp_abort_chunk {
        sctp_chunkhdr_t uh;
} __attribute__((packed)) sctp_abort_chunk_t;


typedef struct sctp_shutdownhdr {
	__be32 cum_tsn_ack;
} __attribute__((packed)) sctp_shutdownhdr_t;

struct sctp_shutdown_chunk_t {
        sctp_chunkhdr_t    chunk_hdr;
        sctp_shutdownhdr_t shutdown_hdr;
} __attribute__ ((packed));

/* RFC 2960.  Section 3.3.10 Operation Error (ERROR) (9) */

typedef struct sctp_errhdr {
	__be16 cause;
	__be16 length;
	__u8  variable[0];
} __attribute__((packed)) sctp_errhdr_t;

typedef struct sctp_operr_chunk {
        sctp_chunkhdr_t chunk_hdr;
	sctp_errhdr_t   err_hdr;
} __attribute__((packed)) sctp_operr_chunk_t;

typedef enum {

	SCTP_ERROR_NO_ERROR	   = cpu_to_be16(0x00),
	SCTP_ERROR_INV_STRM	   = cpu_to_be16(0x01),
	SCTP_ERROR_MISS_PARAM 	   = cpu_to_be16(0x02),
	SCTP_ERROR_STALE_COOKIE	   = cpu_to_be16(0x03),
	SCTP_ERROR_NO_RESOURCE 	   = cpu_to_be16(0x04),
	SCTP_ERROR_DNS_FAILED      = cpu_to_be16(0x05),
	SCTP_ERROR_UNKNOWN_CHUNK   = cpu_to_be16(0x06),
	SCTP_ERROR_INV_PARAM       = cpu_to_be16(0x07),
	SCTP_ERROR_UNKNOWN_PARAM   = cpu_to_be16(0x08),
	SCTP_ERROR_NO_DATA         = cpu_to_be16(0x09),
	SCTP_ERROR_COOKIE_IN_SHUTDOWN = cpu_to_be16(0x0a),


	/* SCTP Implementation Guide:
	 *  11  Restart of an association with new addresses
	 *  12  User Initiated Abort
	 *  13  Protocol Violation
	 */

	SCTP_ERROR_RESTART         = cpu_to_be16(0x0b),
	SCTP_ERROR_USER_ABORT      = cpu_to_be16(0x0c),
	SCTP_ERROR_PROTO_VIOLATION = cpu_to_be16(0x0d),

	/* ADDIP Section 3.3  New Error Causes
	 *
	 * Four new Error Causes are added to the SCTP Operational Errors,
	 * primarily for use in the ASCONF-ACK chunk.
	 *
	 * Value          Cause Code
	 * ---------      ----------------
	 * 0x00A0          Request to Delete Last Remaining IP Address.
	 * 0x00A1          Operation Refused Due to Resource Shortage.
	 * 0x00A2          Request to Delete Source IP Address.
	 * 0x00A3          Association Aborted due to illegal ASCONF-ACK
	 * 0x00A4          Request refused - no authorization.
	 */
	SCTP_ERROR_DEL_LAST_IP	= cpu_to_be16(0x00A0),
	SCTP_ERROR_RSRC_LOW	= cpu_to_be16(0x00A1),
	SCTP_ERROR_DEL_SRC_IP	= cpu_to_be16(0x00A2),
	SCTP_ERROR_ASCONF_ACK   = cpu_to_be16(0x00A3),
	SCTP_ERROR_REQ_REFUSED	= cpu_to_be16(0x00A4),

	/* AUTH Section 4.  New Error Cause
	 *
	 * This section defines a new error cause that will be sent if an AUTH
	 * chunk is received with an unsupported HMAC identifier.
	 * illustrates the new error cause.
	 *
	 * Cause Code      Error Cause Name
	 * --------------------------------------------------------------
	 * 0x0105          Unsupported HMAC Identifier
	 */
	 SCTP_ERROR_UNSUP_HMAC	= cpu_to_be16(0x0105)
} sctp_error_t;



typedef struct sctp_ecnehdr {
	__be32 lowest_tsn;
} sctp_ecnehdr_t;

typedef struct sctp_ecne_chunk {
	sctp_chunkhdr_t chunk_hdr;
	sctp_ecnehdr_t ence_hdr;
} __attribute__((packed)) sctp_ecne_chunk_t;

typedef struct sctp_cwrhdr {
	__be32 lowest_tsn;
} sctp_cwrhdr_t;

typedef struct sctp_cwr_chunk {
	sctp_chunkhdr_t chunk_hdr;
	sctp_cwrhdr_t cwr_hdr;
} __attribute__((packed)) sctp_cwr_chunk_t;

struct sctp_fwdtsn_skip {
	__be16 stream;
	__be16 ssn;
} __attribute__((packed));

struct sctp_fwdtsn_hdr {
	__be32 new_cum_tsn;
	struct sctp_fwdtsn_skip skip[0];
} __attribute((packed));

struct sctp_fwdtsn_chunk {
	struct sctp_chunkhdr chunk_hdr;
	struct sctp_fwdtsn_hdr fwdtsn_hdr;
} __attribute((packed));


typedef struct sctp_addip_param {
	sctp_paramhdr_t	param_hdr;
	__be32		crr_id;
} __attribute__((packed)) sctp_addip_param_t;

typedef struct sctp_addiphdr {
	__be32	serial;
	__u8	params[0];
} __attribute__((packed)) sctp_addiphdr_t;

typedef struct sctp_addip_chunk {
	sctp_chunkhdr_t chunk_hdr;
	sctp_addiphdr_t addip_hdr;
} __attribute__((packed)) sctp_addip_chunk_t;

typedef struct sctp_authhdr {
	__be16 shkey_id;
	__be16 hmac_id;
	__u8   hmac[0];
} __attribute__((packed)) sctp_authhdr_t;

typedef struct sctp_auth_chunk {
	sctp_chunkhdr_t chunk_hdr;
	sctp_authhdr_t auth_hdr;
} __attribute__((packed)) sctp_auth_chunk_t;

#endif /* __LINUX_SCTP_H__ */
