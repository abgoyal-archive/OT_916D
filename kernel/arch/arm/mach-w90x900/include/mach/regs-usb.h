

#ifndef __ASM_ARCH_REGS_USB_H
#define __ASM_ARCH_REGS_USB_H

/* usb Control Registers  */
#define USBH_BA		W90X900_VA_USBEHCIHOST
#define USBD_BA		W90X900_VA_USBDEV
#define USBO_BA		W90X900_VA_USBOHCIHOST

/* USB Host Control Registers */
#define REG_UPSCR0	(USBH_BA+0x064)
#define REG_UPSCR1	(USBH_BA+0x068)
#define REG_USBPCR0	(USBH_BA+0x0C4)
#define REG_USBPCR1	(USBH_BA+0x0C8)

/* USBH OHCI Control Registers */
#define REG_OpModEn	(USBO_BA+0x204)
#define OCALow		0x08

#endif /*  __ASM_ARCH_REGS_USB_H */
