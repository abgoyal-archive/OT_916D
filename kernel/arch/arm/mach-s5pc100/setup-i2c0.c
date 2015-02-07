

#include <linux/kernel.h>
#include <linux/types.h>

struct platform_device; /* don't need the contents */

#include <linux/gpio.h>
#include <plat/iic.h>
#include <plat/gpio-cfg.h>

void s3c_i2c0_cfg_gpio(struct platform_device *dev)
{
	s3c_gpio_cfgpin(S5PC100_GPD(3), S3C_GPIO_SFN(2));
	s3c_gpio_setpull(S5PC100_GPD(3), S3C_GPIO_PULL_UP);
	s3c_gpio_cfgpin(S5PC100_GPD(4), S3C_GPIO_SFN(2));
	s3c_gpio_setpull(S5PC100_GPD(4), S3C_GPIO_PULL_UP);
}
