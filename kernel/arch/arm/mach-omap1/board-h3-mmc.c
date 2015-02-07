

#include <linux/platform_device.h>

#include <linux/i2c/tps65010.h>

#include <plat/mmc.h>
#include <mach/gpio.h>

#include "board-h3.h"

#if defined(CONFIG_MMC_OMAP) || defined(CONFIG_MMC_OMAP_MODULE)

static int mmc_set_power(struct device *dev, int slot, int power_on,
				int vdd)
{
	gpio_set_value(H3_TPS_GPIO_MMC_PWR_EN, power_on);
	return 0;
}

static struct omap_mmc_platform_data mmc1_data = {
	.nr_slots                       = 1,
	.dma_mask			= 0xffffffff,
	.slots[0]       = {
		.set_power              = mmc_set_power,
		.ocr_mask               = MMC_VDD_28_29 | MMC_VDD_30_31 |
					  MMC_VDD_32_33 | MMC_VDD_33_34,
		.name                   = "mmcblk",
	},
};

static struct omap_mmc_platform_data *mmc_data[OMAP16XX_NR_MMC];

void __init h3_mmc_init(void)
{
	int ret;

	ret = gpio_request(H3_TPS_GPIO_MMC_PWR_EN, "MMC power");
	if (ret < 0)
		return;
	gpio_direction_output(H3_TPS_GPIO_MMC_PWR_EN, 0);

	mmc_data[0] = &mmc1_data;
	omap1_init_mmc(mmc_data, OMAP16XX_NR_MMC);
}

#else

void __init h3_mmc_init(void)
{
}

#endif
