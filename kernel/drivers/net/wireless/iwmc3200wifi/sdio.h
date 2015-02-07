

#ifndef __IWM_SDIO_H__
#define __IWM_SDIO_H__

#define IWM_SDIO_DATA_ADDR           0x0
#define IWM_SDIO_INTR_ENABLE_ADDR    0x14
#define IWM_SDIO_INTR_STATUS_ADDR    0x13
#define IWM_SDIO_INTR_CLEAR_ADDR     0x13
#define IWM_SDIO_INTR_GET_SIZE_ADDR  0x2C

#define IWM_SDIO_BLK_SIZE       256

#define iwm_to_if_sdio(i) (struct iwm_sdio_priv *)(iwm->private)

struct iwm_sdio_priv {
	struct sdio_func *func;
	struct iwm_priv *iwm;

	struct workqueue_struct *isr_wq;
	struct work_struct isr_worker;

	struct dentry *cccr_dentry;

	unsigned int blk_size;
};

#endif
