
#ifndef __iwl_fh_h__
#define __iwl_fh_h__

/****************************/
/* Flow Handler Definitions */
/****************************/

#define FH_MEM_LOWER_BOUND                   (0x1000)
#define FH_MEM_UPPER_BOUND                   (0x2000)

#define FH_KW_MEM_ADDR_REG		     (FH_MEM_LOWER_BOUND + 0x97C)


#define FH_MEM_CBBC_LOWER_BOUND          (FH_MEM_LOWER_BOUND + 0x9D0)
#define FH_MEM_CBBC_UPPER_BOUND          (FH_MEM_LOWER_BOUND + 0xA10)

/* Find TFD CB base pointer for given queue (range 0-15). */
#define FH_MEM_CBBC_QUEUE(x)  (FH_MEM_CBBC_LOWER_BOUND + (x) * 0x4)


#define FH_MEM_RSCSR_LOWER_BOUND	(FH_MEM_LOWER_BOUND + 0xBC0)
#define FH_MEM_RSCSR_UPPER_BOUND	(FH_MEM_LOWER_BOUND + 0xC00)
#define FH_MEM_RSCSR_CHNL0		(FH_MEM_RSCSR_LOWER_BOUND)

#define FH_RSCSR_CHNL0_STTS_WPTR_REG	(FH_MEM_RSCSR_CHNL0)

#define FH_RSCSR_CHNL0_RBDCB_BASE_REG	(FH_MEM_RSCSR_CHNL0 + 0x004)

#define FH_RSCSR_CHNL0_RBDCB_WPTR_REG	(FH_MEM_RSCSR_CHNL0 + 0x008)
#define FH_RSCSR_CHNL0_WPTR        (FH_RSCSR_CHNL0_RBDCB_WPTR_REG)


#define FH_MEM_RCSR_LOWER_BOUND      (FH_MEM_LOWER_BOUND + 0xC00)
#define FH_MEM_RCSR_UPPER_BOUND      (FH_MEM_LOWER_BOUND + 0xCC0)
#define FH_MEM_RCSR_CHNL0            (FH_MEM_RCSR_LOWER_BOUND)

#define FH_MEM_RCSR_CHNL0_CONFIG_REG	(FH_MEM_RCSR_CHNL0)

#define FH_RCSR_CHNL0_RX_CONFIG_RB_TIMEOUT_MSK (0x00000FF0) /* bits 4-11 */
#define FH_RCSR_CHNL0_RX_CONFIG_IRQ_DEST_MSK   (0x00001000) /* bits 12 */
#define FH_RCSR_CHNL0_RX_CONFIG_SINGLE_FRAME_MSK (0x00008000) /* bit 15 */
#define FH_RCSR_CHNL0_RX_CONFIG_RB_SIZE_MSK   (0x00030000) /* bits 16-17 */
#define FH_RCSR_CHNL0_RX_CONFIG_RBDBC_SIZE_MSK (0x00F00000) /* bits 20-23 */
#define FH_RCSR_CHNL0_RX_CONFIG_DMA_CHNL_EN_MSK (0xC0000000) /* bits 30-31*/

#define FH_RCSR_RX_CONFIG_RBDCB_SIZE_POS	(20)
#define FH_RCSR_RX_CONFIG_REG_IRQ_RBTH_POS	(4)
#define RX_RB_TIMEOUT	(0x10)

#define FH_RCSR_RX_CONFIG_CHNL_EN_PAUSE_VAL         (0x00000000)
#define FH_RCSR_RX_CONFIG_CHNL_EN_PAUSE_EOF_VAL     (0x40000000)
#define FH_RCSR_RX_CONFIG_CHNL_EN_ENABLE_VAL        (0x80000000)

#define FH_RCSR_RX_CONFIG_REG_VAL_RB_SIZE_4K    (0x00000000)
#define FH_RCSR_RX_CONFIG_REG_VAL_RB_SIZE_8K    (0x00010000)
#define FH_RCSR_RX_CONFIG_REG_VAL_RB_SIZE_12K   (0x00020000)
#define FH_RCSR_RX_CONFIG_REG_VAL_RB_SIZE_16K   (0x00030000)

#define FH_RCSR_CHNL0_RX_IGNORE_RXF_EMPTY              (0x00000004)
#define FH_RCSR_CHNL0_RX_CONFIG_IRQ_DEST_NO_INT_VAL    (0x00000000)
#define FH_RCSR_CHNL0_RX_CONFIG_IRQ_DEST_INT_HOST_VAL  (0x00001000)

#define FH_RSCSR_FRAME_SIZE_MSK	(0x00003FFF)	/* bits 0-13 */

#define FH_MEM_RSSR_LOWER_BOUND           (FH_MEM_LOWER_BOUND + 0xC40)
#define FH_MEM_RSSR_UPPER_BOUND           (FH_MEM_LOWER_BOUND + 0xD00)

#define FH_MEM_RSSR_SHARED_CTRL_REG       (FH_MEM_RSSR_LOWER_BOUND)
#define FH_MEM_RSSR_RX_STATUS_REG	(FH_MEM_RSSR_LOWER_BOUND + 0x004)
#define FH_MEM_RSSR_RX_ENABLE_ERR_IRQ2DRV\
					(FH_MEM_RSSR_LOWER_BOUND + 0x008)

#define FH_RSSR_CHNL0_RX_STATUS_CHNL_IDLE	(0x01000000)

#define FH_MEM_TFDIB_REG1_ADDR_BITSHIFT	28

/* TFDB  Area - TFDs buffer table */
#define FH_MEM_TFDIB_DRAM_ADDR_LSB_MSK      (0xFFFFFFFF)
#define FH_TFDIB_LOWER_BOUND       (FH_MEM_LOWER_BOUND + 0x900)
#define FH_TFDIB_UPPER_BOUND       (FH_MEM_LOWER_BOUND + 0x958)
#define FH_TFDIB_CTRL0_REG(_chnl)  (FH_TFDIB_LOWER_BOUND + 0x8 * (_chnl))
#define FH_TFDIB_CTRL1_REG(_chnl)  (FH_TFDIB_LOWER_BOUND + 0x8 * (_chnl) + 0x4)

#define FH_TCSR_LOWER_BOUND  (FH_MEM_LOWER_BOUND + 0xD00)
#define FH_TCSR_UPPER_BOUND  (FH_MEM_LOWER_BOUND + 0xE60)

/* Find Control/Status reg for given Tx DMA/FIFO channel */
#define FH49_TCSR_CHNL_NUM                            (7)
#define FH50_TCSR_CHNL_NUM                            (8)

/* TCSR: tx_config register values */
#define FH_TCSR_CHNL_TX_CONFIG_REG(_chnl)	\
		(FH_TCSR_LOWER_BOUND + 0x20 * (_chnl))
#define FH_TCSR_CHNL_TX_CREDIT_REG(_chnl)	\
		(FH_TCSR_LOWER_BOUND + 0x20 * (_chnl) + 0x4)
#define FH_TCSR_CHNL_TX_BUF_STS_REG(_chnl)	\
		(FH_TCSR_LOWER_BOUND + 0x20 * (_chnl) + 0x8)

#define FH_TCSR_TX_CONFIG_REG_VAL_MSG_MODE_TXF		(0x00000000)
#define FH_TCSR_TX_CONFIG_REG_VAL_MSG_MODE_DRV		(0x00000001)

#define FH_TCSR_TX_CONFIG_REG_VAL_DMA_CREDIT_DISABLE	(0x00000000)
#define FH_TCSR_TX_CONFIG_REG_VAL_DMA_CREDIT_ENABLE	(0x00000008)

#define FH_TCSR_TX_CONFIG_REG_VAL_CIRQ_HOST_NOINT	(0x00000000)
#define FH_TCSR_TX_CONFIG_REG_VAL_CIRQ_HOST_ENDTFD	(0x00100000)
#define FH_TCSR_TX_CONFIG_REG_VAL_CIRQ_HOST_IFTFD	(0x00200000)

#define FH_TCSR_TX_CONFIG_REG_VAL_CIRQ_RTC_NOINT	(0x00000000)
#define FH_TCSR_TX_CONFIG_REG_VAL_CIRQ_RTC_ENDTFD	(0x00400000)
#define FH_TCSR_TX_CONFIG_REG_VAL_CIRQ_RTC_IFTFD	(0x00800000)

#define FH_TCSR_TX_CONFIG_REG_VAL_DMA_CHNL_PAUSE	(0x00000000)
#define FH_TCSR_TX_CONFIG_REG_VAL_DMA_CHNL_PAUSE_EOF	(0x40000000)
#define FH_TCSR_TX_CONFIG_REG_VAL_DMA_CHNL_ENABLE	(0x80000000)

#define FH_TCSR_CHNL_TX_BUF_STS_REG_VAL_TFDB_EMPTY	(0x00000000)
#define FH_TCSR_CHNL_TX_BUF_STS_REG_VAL_TFDB_WAIT	(0x00002000)
#define FH_TCSR_CHNL_TX_BUF_STS_REG_VAL_TFDB_VALID	(0x00000003)

#define FH_TCSR_CHNL_TX_BUF_STS_REG_POS_TB_NUM		(20)
#define FH_TCSR_CHNL_TX_BUF_STS_REG_POS_TB_IDX		(12)

#define FH_TSSR_LOWER_BOUND		(FH_MEM_LOWER_BOUND + 0xEA0)
#define FH_TSSR_UPPER_BOUND		(FH_MEM_LOWER_BOUND + 0xEC0)

#define FH_TSSR_TX_STATUS_REG		(FH_TSSR_LOWER_BOUND + 0x010)

#define FH_TSSR_TX_ERROR_REG		(FH_TSSR_LOWER_BOUND + 0x018)

#define FH_TSSR_TX_STATUS_REG_BIT_BUFS_EMPTY(_chnl) ((1 << (_chnl)) << 24)
#define FH_TSSR_TX_STATUS_REG_BIT_NO_PEND_REQ(_chnl) ((1 << (_chnl)) << 16)

#define FH_TSSR_TX_STATUS_REG_MSK_CHNL_IDLE(_chnl) \
	(FH_TSSR_TX_STATUS_REG_BIT_BUFS_EMPTY(_chnl) | \
	FH_TSSR_TX_STATUS_REG_BIT_NO_PEND_REQ(_chnl))

/* Tx service channels */
#define FH_SRVC_CHNL		(9)
#define FH_SRVC_LOWER_BOUND	(FH_MEM_LOWER_BOUND + 0x9C8)
#define FH_SRVC_UPPER_BOUND	(FH_MEM_LOWER_BOUND + 0x9D0)
#define FH_SRVC_CHNL_SRAM_ADDR_REG(_chnl) \
		(FH_SRVC_LOWER_BOUND + ((_chnl) - 9) * 0x4)

#define FH_TX_CHICKEN_BITS_REG	(FH_MEM_LOWER_BOUND + 0xE98)
#define FH_TX_CHICKEN_BITS_SCD_AUTO_RETRY_EN	(0x00000002)

#define RX_QUEUE_SIZE                         256
#define RX_QUEUE_MASK                         255
#define RX_QUEUE_SIZE_LOG                     8

#define RX_FREE_BUFFERS 64
#define RX_LOW_WATERMARK 8

/* Size of one Rx buffer in host DRAM */
#define IWL_RX_BUF_SIZE_3K (3 * 1000) /* 3945 only */
#define IWL_RX_BUF_SIZE_4K (4 * 1024)
#define IWL_RX_BUF_SIZE_8K (8 * 1024)

struct iwl_rb_status {
	__le16 closed_rb_num;
	__le16 closed_fr_num;
	__le16 finished_rb_num;
	__le16 finished_fr_nam;
	__le32 __unused; /* 3945 only */
} __attribute__ ((packed));


#define TFD_QUEUE_SIZE_MAX      (256)
#define TFD_QUEUE_SIZE_BC_DUP	(64)
#define TFD_QUEUE_BC_SIZE	(TFD_QUEUE_SIZE_MAX + TFD_QUEUE_SIZE_BC_DUP)
#define IWL_TX_DMA_MASK        DMA_BIT_MASK(36)
#define IWL_NUM_OF_TBS		20

static inline u8 iwl_get_dma_hi_addr(dma_addr_t addr)
{
	return (sizeof(addr) > sizeof(u32) ? (addr >> 16) >> 16 : 0) & 0xF;
}
struct iwl_tfd_tb {
	__le32 lo;
	__le16 hi_n_len;
} __attribute__((packed));

struct iwl_tfd {
	u8 __reserved1[3];
	u8 num_tbs;
	struct iwl_tfd_tb tbs[IWL_NUM_OF_TBS];
	__le32 __pad;
} __attribute__ ((packed));

/* Keep Warm Size */
#define IWL_KW_SIZE 0x1000	/* 4k */

#endif /* !__iwl_fh_h__ */
