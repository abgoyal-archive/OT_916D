

#ifndef _BF5XX_I2S_PCM_H
#define _BF5XX_I2S_PCM_H

struct bf5xx_pcm_dma_params {
	char *name;			/* stream identifier */
};

struct bf5xx_gpio {
	u32 sys;
	u32 rx;
	u32 tx;
	u32 clk;
	u32 frm;
};

/* platform data */
extern struct snd_soc_platform bf5xx_i2s_soc_platform;

#endif
