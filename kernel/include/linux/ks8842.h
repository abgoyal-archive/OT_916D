

#ifndef _LINUX_KS8842_H
#define _LINUX_KS8842_H

#include <linux/if_ether.h>

struct ks8842_platform_data {
	u8 macaddr[ETH_ALEN];
};

#endif
