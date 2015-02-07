

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/pci.h>
#include "shpchp.h"


/* A few routines that create sysfs entries for the hot plug controller */

static ssize_t show_ctrl (struct device *dev, struct device_attribute *attr, char *buf)
{
	struct pci_dev *pdev;
	char * out = buf;
	int index, busnr;
	struct resource *res;
	struct pci_bus *bus;

	pdev = container_of (dev, struct pci_dev, dev);
	bus = pdev->subordinate;

	out += sprintf(buf, "Free resources: memory\n");
	pci_bus_for_each_resource(bus, res, index) {
		if (res && (res->flags & IORESOURCE_MEM) &&
				!(res->flags & IORESOURCE_PREFETCH)) {
			out += sprintf(out, "start = %8.8llx, "
					"length = %8.8llx\n",
					(unsigned long long)res->start,
					(unsigned long long)(res->end - res->start));
		}
	}
	out += sprintf(out, "Free resources: prefetchable memory\n");
	pci_bus_for_each_resource(bus, res, index) {
		if (res && (res->flags & IORESOURCE_MEM) &&
			       (res->flags & IORESOURCE_PREFETCH)) {
			out += sprintf(out, "start = %8.8llx, "
					"length = %8.8llx\n",
					(unsigned long long)res->start,
					(unsigned long long)(res->end - res->start));
		}
	}
	out += sprintf(out, "Free resources: IO\n");
	pci_bus_for_each_resource(bus, res, index) {
		if (res && (res->flags & IORESOURCE_IO)) {
			out += sprintf(out, "start = %8.8llx, "
					"length = %8.8llx\n",
					(unsigned long long)res->start,
					(unsigned long long)(res->end - res->start));
		}
	}
	out += sprintf(out, "Free resources: bus numbers\n");
	for (busnr = bus->secondary; busnr <= bus->subordinate; busnr++) {
		if (!pci_find_bus(pci_domain_nr(bus), busnr))
			break;
	}
	if (busnr < bus->subordinate)
		out += sprintf(out, "start = %8.8x, length = %8.8x\n",
				busnr, (bus->subordinate - busnr));

	return out - buf;
}
static DEVICE_ATTR (ctrl, S_IRUGO, show_ctrl, NULL);

int __must_check shpchp_create_ctrl_files (struct controller *ctrl)
{
	return device_create_file (&ctrl->pci_dev->dev, &dev_attr_ctrl);
}

void shpchp_remove_ctrl_files(struct controller *ctrl)
{
	device_remove_file(&ctrl->pci_dev->dev, &dev_attr_ctrl);
}
