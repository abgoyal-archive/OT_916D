
#ifndef __ASM_ARCH_AUTCPU12_H
#define __ASM_ARCH_AUTCPU12_H

#define AUTCPU12_PHYS_CS8900A		CS2_PHYS_BASE		/* physical */
#define AUTCPU12_VIRT_CS8900A		(0xfe000000)		/* virtual */

#define AUTCPU12_PHYS_FLASH		CS0_PHYS_BASE		/* physical */

/* offset for device specific information structure */
#define AUTCPU12_LCDINFO_OFFS		(0x00010000)	
#define AUTCPU12_PHYS_VIDEO		CS6_PHYS_BASE
#define AUTCPU12_VIRT_VIDEO		(0xfd000000)

#define AUTCPU12_PHYS_CHAR_LCD         	CS1_PHYS_BASE +0x00000000  /* physical */

#define AUTCPU12_PHYS_NVRAM            	CS1_PHYS_BASE +0x02000000  /* physical */

#define AUTCPU12_PHYS_CSAUX1           	CS1_PHYS_BASE +0x04000000  /* physical */

#define AUTCPU12_PHYS_SMC              	CS1_PHYS_BASE +0x06000000  /* physical */

#define AUTCPU12_PHYS_CAN              	CS1_PHYS_BASE +0x08000000  /* physical */

#define AUTCPU12_PHYS_TOUCH            	CS1_PHYS_BASE +0x0A000000  /* physical */

#define AUTCPU12_PHYS_IO               	CS1_PHYS_BASE +0x0C000000  /* physical */

#define AUTCPU12_PHYS_LPT              	CS1_PHYS_BASE +0x0E000000  /* physical */

#define AUTCPU12_SMC_RDY		(1<<2)
#define AUTCPU12_SMC_ALE		(1<<3)
#define AUTCPU12_SMC_CLE  		(1<<4)
#define AUTCPU12_SMC_PORT_OFFSET	PBDR
#define AUTCPU12_SMC_SELECT_OFFSET 	0x10
#define AUTCPU12_DPOT_PORT_OFFSET	PEDR
#define	AUTCPU12_DPOT_CS		(1<<0)
#define AUTCPU12_DPOT_CLK    		(1<<1)
#define	AUTCPU12_DPOT_UD		(1<<2)

#endif
