

#ifndef _ATMEL_SSC_DAI_H
#define _ATMEL_SSC_DAI_H

#include <linux/types.h>
#include <linux/atmel-ssc.h>

#include "atmel-pcm.h"

/* SSC system clock ids */
#define ATMEL_SYSCLK_MCK	0 /* SSC uses AT91 MCK as system clock */

/* SSC divider ids */
#define ATMEL_SSC_CMR_DIV	0 /* MCK divider for BCLK */
#define ATMEL_SSC_TCMR_PERIOD	1 /* BCLK divider for transmit FS */
#define ATMEL_SSC_RCMR_PERIOD	2 /* BCLK divider for receive FS */
#define SSC_DIR_MASK_UNUSED	0
#define SSC_DIR_MASK_PLAYBACK	1
#define SSC_DIR_MASK_CAPTURE	2

/* START bit field values */
#define SSC_START_CONTINUOUS	0
#define SSC_START_TX_RX		1
#define SSC_START_LOW_RF	2
#define SSC_START_HIGH_RF	3
#define SSC_START_FALLING_RF	4
#define SSC_START_RISING_RF	5
#define SSC_START_LEVEL_RF	6
#define SSC_START_EDGE_RF	7
#define SSS_START_COMPARE_0	8

/* CKI bit field values */
#define SSC_CKI_FALLING		0
#define SSC_CKI_RISING		1

/* CKO bit field values */
#define SSC_CKO_NONE		0
#define SSC_CKO_CONTINUOUS	1
#define SSC_CKO_TRANSFER	2

/* CKS bit field values */
#define SSC_CKS_DIV		0
#define SSC_CKS_CLOCK		1
#define SSC_CKS_PIN		2

/* FSEDGE bit field values */
#define SSC_FSEDGE_POSITIVE	0
#define SSC_FSEDGE_NEGATIVE	1

/* FSOS bit field values */
#define SSC_FSOS_NONE		0
#define SSC_FSOS_NEGATIVE	1
#define SSC_FSOS_POSITIVE	2
#define SSC_FSOS_LOW		3
#define SSC_FSOS_HIGH		4
#define SSC_FSOS_TOGGLE		5

#define START_DELAY		1

struct atmel_ssc_state {
	u32 ssc_cmr;
	u32 ssc_rcmr;
	u32 ssc_rfmr;
	u32 ssc_tcmr;
	u32 ssc_tfmr;
	u32 ssc_sr;
	u32 ssc_imr;
};


struct atmel_ssc_info {
	char *name;
	struct ssc_device *ssc;
	spinlock_t lock;	/* lock for dir_mask */
	unsigned short dir_mask;	/* 0=unused, 1=playback, 2=capture */
	unsigned short initialized;	/* true if SSC has been initialized */
	unsigned short daifmt;
	unsigned short cmr_div;
	unsigned short tcmr_period;
	unsigned short rcmr_period;
	struct atmel_pcm_dma_params *dma_params[2];
	struct atmel_ssc_state ssc_state;
};
extern struct snd_soc_dai atmel_ssc_dai[];

#endif /* _AT91_SSC_DAI_H */
