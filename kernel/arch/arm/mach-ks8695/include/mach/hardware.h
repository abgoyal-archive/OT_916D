

#ifndef __ASM_ARCH_HARDWARE_H
#define __ASM_ARCH_HARDWARE_H

#include <asm/sizes.h>

#define KS8695_CLOCK_RATE	25000000

#define KS8695_SDRAM_PA		0x00000000


#define KS8695_IO_PA		0x03F00000
#define KS8695_IO_VA		0xF0000000
#define KS8695_IO_SIZE		SZ_1M

#define KS8695_PCIMEM_PA	0x60000000
#define KS8695_PCIMEM_SIZE	SZ_512M

#define KS8695_PCIIO_PA		0x80000000
#define KS8695_PCIIO_SIZE	SZ_64K


#define pcibios_assign_all_busses()	1

#define PCIBIOS_MIN_IO		0
#define PCIBIOS_MIN_MEM		0

#endif
