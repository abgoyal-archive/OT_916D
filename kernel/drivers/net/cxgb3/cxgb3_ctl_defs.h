
#ifndef _CXGB3_OFFLOAD_CTL_DEFS_H
#define _CXGB3_OFFLOAD_CTL_DEFS_H

enum {
	GET_MAX_OUTSTANDING_WR 	= 0,
	GET_TX_MAX_CHUNK	= 1,
	GET_TID_RANGE		= 2,
	GET_STID_RANGE		= 3,
	GET_RTBL_RANGE		= 4,
	GET_L2T_CAPACITY	= 5,
	GET_MTUS		= 6,
	GET_WR_LEN		= 7,
	GET_IFF_FROM_MAC	= 8,
	GET_DDP_PARAMS		= 9,
	GET_PORTS		= 10,

	ULP_ISCSI_GET_PARAMS	= 11,
	ULP_ISCSI_SET_PARAMS	= 12,

	RDMA_GET_PARAMS		= 13,
	RDMA_CQ_OP		= 14,
	RDMA_CQ_SETUP		= 15,
	RDMA_CQ_DISABLE		= 16,
	RDMA_CTRL_QP_SETUP	= 17,
	RDMA_GET_MEM		= 18,
	RDMA_GET_MIB		= 19,

	GET_RX_PAGE_INFO	= 50,
	GET_ISCSI_IPV4ADDR	= 51,

	GET_EMBEDDED_INFO	= 70,
};

struct tid_range {
	unsigned int base;	/* first TID */
	unsigned int num;	/* number of TIDs in range */
};

struct mtutab {
	unsigned int size;	/* # of entries in the MTU table */
	const unsigned short *mtus;	/* the MTU table values */
};

struct net_device;

struct iff_mac {
	struct net_device *dev;	/* the net_device */
	const unsigned char *mac_addr;	/* MAC address to lookup */
	u16 vlan_tag;
};

/* Structure used to request a port's iSCSI IPv4 address */
struct iscsi_ipv4addr {
	struct net_device *dev;	/* the net_device */
	__be32 ipv4addr;	/* the return iSCSI IPv4 address */
};

struct pci_dev;

struct ddp_params {
	unsigned int llimit;	/* TDDP region start address */
	unsigned int ulimit;	/* TDDP region end address */
	unsigned int tag_mask;	/* TDDP tag mask */
	struct pci_dev *pdev;
};

struct adap_ports {
	unsigned int nports;	/* number of ports on this adapter */
	struct net_device *lldevs[2];
};

struct ulp_iscsi_info {
	unsigned int offset;
	unsigned int llimit;
	unsigned int ulimit;
	unsigned int tagmask;
	u8 pgsz_factor[4];
	unsigned int max_rxsz;
	unsigned int max_txsz;
	struct pci_dev *pdev;
};

struct rdma_info {
	unsigned int tpt_base;	/* TPT base address */
	unsigned int tpt_top;	/* TPT last entry address */
	unsigned int pbl_base;	/* PBL base address */
	unsigned int pbl_top;	/* PBL last entry address */
	unsigned int rqt_base;	/* RQT base address */
	unsigned int rqt_top;	/* RQT last entry address */
	unsigned int udbell_len;	/* user doorbell region length */
	unsigned long udbell_physbase;	/* user doorbell physical start addr */
	void __iomem *kdb_addr;	/* kernel doorbell register address */
	struct pci_dev *pdev;	/* associated PCI device */
};

struct rdma_cq_op {
	unsigned int id;
	unsigned int op;
	unsigned int credits;
};

struct rdma_cq_setup {
	unsigned int id;
	unsigned long long base_addr;
	unsigned int size;
	unsigned int credits;
	unsigned int credit_thres;
	unsigned int ovfl_mode;
};

struct rdma_ctrlqp_setup {
	unsigned long long base_addr;
	unsigned int size;
};

struct ofld_page_info {
	unsigned int page_size;  /* Page size, should be a power of 2 */
	unsigned int num;        /* Number of pages */
};

struct ch_embedded_info {
	u32 fw_vers;
	u32 tp_vers;
};
#endif				/* _CXGB3_OFFLOAD_CTL_DEFS_H */
