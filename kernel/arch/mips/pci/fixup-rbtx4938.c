
#include <linux/types.h>
#include <asm/txx9/pci.h>
#include <asm/txx9/rbtx4938.h>

int __init rbtx4938_pci_map_irq(const struct pci_dev *dev, u8 slot, u8 pin)
{
	int irq = tx4938_pcic1_map_irq(dev, slot);

	if (irq >= 0)
		return irq;
	irq = pin;
	/* IRQ rotation */
	irq--;	/* 0-3 */
	if (slot == TX4927_PCIC_IDSEL_AD_TO_SLOT(23)) {
		/* PCI CardSlot (IDSEL=A23) */
		/* PCIA => PCIA (IDSEL=A23) */
		irq = (irq + 0 + slot) % 4;
	} else {
		/* PCI Backplane */
		if (txx9_pci_option & TXX9_PCI_OPT_PICMG)
			irq = (irq + 33 - slot) % 4;
		else
			irq = (irq + 3 + slot) % 4;
	}
	irq++;	/* 1-4 */

	switch (irq) {
	case 1:
		irq = RBTX4938_IRQ_IOC_PCIA;
		break;
	case 2:
		irq = RBTX4938_IRQ_IOC_PCIB;
		break;
	case 3:
		irq = RBTX4938_IRQ_IOC_PCIC;
		break;
	case 4:
		irq = RBTX4938_IRQ_IOC_PCID;
		break;
	}
	return irq;
}
