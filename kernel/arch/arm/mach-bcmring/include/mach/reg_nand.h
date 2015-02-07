


#if !defined(__ASM_ARCH_REG_NAND_H)
#define __ASM_ARCH_REG_NAND_H

/* ---- Include Files ---------------------------------------------------- */
#include <csp/reg.h>
#include <mach/reg_umi.h>

/* ---- Constants and Types ---------------------------------------------- */

#define HW_NAND_BASE       MM_IO_BASE_NAND	/* NAND Flash */

/* DMA accesses by the bootstrap need hard nonvirtual addresses */
#define REG_NAND_CMD            __REG16(HW_NAND_BASE + 0)
#define REG_NAND_ADDR           __REG16(HW_NAND_BASE + 4)

#define REG_NAND_PHYS_DATA16   (HW_NAND_BASE + 8)
#define REG_NAND_PHYS_DATA8    (HW_NAND_BASE + 8)
#define REG_NAND_DATA16         __REG16(REG_NAND_PHYS_DATA16)
#define REG_NAND_DATA8          __REG8(REG_NAND_PHYS_DATA8)

/* use appropriate offset to make sure it start at the 1K boundary */
#define REG_NAND_PHYS_DATA_DMA   (HW_NAND_BASE + 0x400)
#define REG_NAND_DATA_DMA         __REG32(REG_NAND_PHYS_DATA_DMA)

/* Linux DMA requires physical address of the data register */
#define REG_NAND_DATA16_PADDR    HW_IO_VIRT_TO_PHYS(REG_NAND_PHYS_DATA16)
#define REG_NAND_DATA8_PADDR     HW_IO_VIRT_TO_PHYS(REG_NAND_PHYS_DATA8)
#define REG_NAND_DATA_PADDR      HW_IO_VIRT_TO_PHYS(REG_NAND_PHYS_DATA_DMA)

#define NAND_BUS_16BIT()        (0)
#define NAND_BUS_8BIT()         (!NAND_BUS_16BIT())

/* Register offsets */
#define REG_NAND_CMD_OFFSET     (0)
#define REG_NAND_ADDR_OFFSET    (4)
#define REG_NAND_DATA8_OFFSET   (8)

#endif
