

#include <asm/addrspace.h>

#define MAXINEFB_IMS332_ADDRESS		KSEG1ADDR(0x1c140000)

#define DS5000_xx_ONBOARD_FBMEM_START	KSEG1ADDR(0x0a000000)


#define IMS332_REG_CURSOR_RAM           0x200	/* hardware cursor bitmap */

#define IMS332_REG_COLOR_PALETTE        0x100	/* color palette, 256 entries */
#define IMS332_REG_CURSOR_COLOR_PALETTE	0x0a1	/* cursor color palette, */
						/* 3 entries             */
