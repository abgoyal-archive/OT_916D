

#include <linux/dma-mapping.h>
#include <linux/pfn.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/pci.h>

#include <asm/machdep.h>
#include <asm/swiotlb.h>
#include <asm/dma.h>
#include <asm/abs_addr.h>

unsigned int ppc_swiotlb_enable;

struct dma_map_ops swiotlb_dma_ops = {
	.alloc_coherent = dma_direct_alloc_coherent,
	.free_coherent = dma_direct_free_coherent,
	.map_sg = swiotlb_map_sg_attrs,
	.unmap_sg = swiotlb_unmap_sg_attrs,
	.dma_supported = swiotlb_dma_supported,
	.map_page = swiotlb_map_page,
	.unmap_page = swiotlb_unmap_page,
	.sync_single_for_cpu = swiotlb_sync_single_for_cpu,
	.sync_single_for_device = swiotlb_sync_single_for_device,
	.sync_sg_for_cpu = swiotlb_sync_sg_for_cpu,
	.sync_sg_for_device = swiotlb_sync_sg_for_device,
	.mapping_error = swiotlb_dma_mapping_error,
};

void pci_dma_dev_setup_swiotlb(struct pci_dev *pdev)
{
	struct pci_controller *hose;
	struct dev_archdata *sd;

	hose = pci_bus_to_host(pdev->bus);
	sd = &pdev->dev.archdata;
	sd->max_direct_dma_addr =
		hose->dma_window_base_cur + hose->dma_window_size;
}

static int ppc_swiotlb_bus_notify(struct notifier_block *nb,
				  unsigned long action, void *data)
{
	struct device *dev = data;
	struct dev_archdata *sd;

	/* We are only intereted in device addition */
	if (action != BUS_NOTIFY_ADD_DEVICE)
		return 0;

	sd = &dev->archdata;
	sd->max_direct_dma_addr = 0;

	/* May need to bounce if the device can't address all of DRAM */
	if ((dma_get_mask(dev) + 1) < memblock_end_of_DRAM())
		set_dma_ops(dev, &swiotlb_dma_ops);

	return NOTIFY_DONE;
}

static struct notifier_block ppc_swiotlb_plat_bus_notifier = {
	.notifier_call = ppc_swiotlb_bus_notify,
	.priority = 0,
};

static struct notifier_block ppc_swiotlb_of_bus_notifier = {
	.notifier_call = ppc_swiotlb_bus_notify,
	.priority = 0,
};

int __init swiotlb_setup_bus_notifier(void)
{
	bus_register_notifier(&platform_bus_type,
			      &ppc_swiotlb_plat_bus_notifier);
	bus_register_notifier(&of_platform_bus_type,
			      &ppc_swiotlb_of_bus_notifier);

	return 0;
}
