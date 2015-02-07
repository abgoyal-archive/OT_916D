

#ifndef __rio_pci_h__
#define	__rio_pci_h__


#define	PCITpFastClock		0x80
#define	PCITpSlowClock		0x00
#define	PCITpFastLinks	        0x40
#define	PCITpSlowLinks	        0x00
#define	PCITpIntEnable		0x04
#define	PCITpIntDisable		0x00
#define	PCITpBusEnable		0x02
#define	PCITpBusDisable		0x00
#define	PCITpBootFromRam	0x01
#define	PCITpBootFromLink	0x00

#define	RIO_PCI_VENDOR		0x11CB
#define	RIO_PCI_DEVICE		0x8000
#define	RIO_PCI_BASE_CLASS	0x02
#define	RIO_PCI_SUB_CLASS	0x80
#define	RIO_PCI_PROG_IFACE	0x00

#define RIO_PCI_RID		0x0008
#define RIO_PCI_BADR0		0x0010
#define RIO_PCI_INTLN		0x003C
#define RIO_PCI_INTPIN		0x003D

#define	RIO_PCI_MEM_SIZE	65536

#define	RIO_PCI_TURBO_TP	0x80
#define	RIO_PCI_FAST_LINKS	0x40
#define	RIO_PCI_INT_ENABLE	0x04
#define	RIO_PCI_TP_BUS_ENABLE	0x02
#define	RIO_PCI_BOOT_FROM_RAM	0x01

#define	RIO_PCI_DEFAULT_MODE	0x05

#endif				/* __rio_pci_h__ */
