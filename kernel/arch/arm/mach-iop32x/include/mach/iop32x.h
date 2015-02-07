

#ifndef __IOP32X_H
#define __IOP32X_H

#define IOP3XX_GPIO_REG(reg)	(IOP3XX_PERIPHERAL_VIRT_BASE + 0x07c4 + (reg))
#define IOP3XX_TIMER_REG(reg)	(IOP3XX_PERIPHERAL_VIRT_BASE + 0x07e0 + (reg))

#include <asm/hardware/iop3xx.h>

#define IOP32X_MAX_RAM_SIZE            0x40000000UL
#define IOP3XX_MAX_RAM_SIZE            IOP32X_MAX_RAM_SIZE
#define IOP3XX_PCI_LOWER_MEM_BA        0x80000000

#endif
