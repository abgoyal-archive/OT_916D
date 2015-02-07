

#include <linux/pci.h>
#include <asm/pci-bridge.h>
#include <asm/prom.h>

static u32 get_int_prop(struct device_node *np, const char *name, u32 def)
{
	const u32 *prop;
	int len;

	prop = of_get_property(np, name, &len);
	if (prop && len >= 4)
		return *prop;
	return def;
}

unsigned int pci_parse_of_flags(u32 addr0, int bridge)
{
	unsigned int flags = 0;

	if (addr0 & 0x02000000) {
		flags = IORESOURCE_MEM | PCI_BASE_ADDRESS_SPACE_MEMORY;
		flags |= (addr0 >> 22) & PCI_BASE_ADDRESS_MEM_TYPE_64;
		flags |= (addr0 >> 28) & PCI_BASE_ADDRESS_MEM_TYPE_1M;
		if (addr0 & 0x40000000)
			flags |= IORESOURCE_PREFETCH
				 | PCI_BASE_ADDRESS_MEM_PREFETCH;
		/* Note: We don't know whether the ROM has been left enabled
		 * by the firmware or not. We mark it as disabled (ie, we do
		 * not set the IORESOURCE_ROM_ENABLE flag) for now rather than
		 * do a config space read, it will be force-enabled if needed
		 */
		if (!bridge && (addr0 & 0xff) == 0x30)
			flags |= IORESOURCE_READONLY;
	} else if (addr0 & 0x01000000)
		flags = IORESOURCE_IO | PCI_BASE_ADDRESS_SPACE_IO;
	if (flags)
		flags |= IORESOURCE_SIZEALIGN;
	return flags;
}

static void of_pci_parse_addrs(struct device_node *node, struct pci_dev *dev)
{
	u64 base, size;
	unsigned int flags;
	struct resource *res;
	const u32 *addrs;
	u32 i;
	int proplen;

	addrs = of_get_property(node, "assigned-addresses", &proplen);
	if (!addrs)
		return;
	pr_debug("    parse addresses (%d bytes) @ %p\n", proplen, addrs);
	for (; proplen >= 20; proplen -= 20, addrs += 5) {
		flags = pci_parse_of_flags(addrs[0], 0);
		if (!flags)
			continue;
		base = of_read_number(&addrs[1], 2);
		size = of_read_number(&addrs[3], 2);
		if (!size)
			continue;
		i = addrs[0] & 0xff;
		pr_debug("  base: %llx, size: %llx, i: %x\n",
			 (unsigned long long)base,
			 (unsigned long long)size, i);

		if (PCI_BASE_ADDRESS_0 <= i && i <= PCI_BASE_ADDRESS_5) {
			res = &dev->resource[(i - PCI_BASE_ADDRESS_0) >> 2];
		} else if (i == dev->rom_base_reg) {
			res = &dev->resource[PCI_ROM_RESOURCE];
			flags |= IORESOURCE_READONLY | IORESOURCE_CACHEABLE;
		} else {
			printk(KERN_ERR "PCI: bad cfg reg num 0x%x\n", i);
			continue;
		}
		res->start = base;
		res->end = base + size - 1;
		res->flags = flags;
		res->name = pci_name(dev);
	}
}

struct pci_dev *of_create_pci_dev(struct device_node *node,
				 struct pci_bus *bus, int devfn)
{
	struct pci_dev *dev;
	const char *type;
	struct pci_slot *slot;

	dev = alloc_pci_dev();
	if (!dev)
		return NULL;
	type = of_get_property(node, "device_type", NULL);
	if (type == NULL)
		type = "";

	pr_debug("    create device, devfn: %x, type: %s\n", devfn, type);

	dev->bus = bus;
	dev->sysdata = node;
	dev->dev.parent = bus->bridge;
	dev->dev.bus = &pci_bus_type;
	dev->devfn = devfn;
	dev->multifunction = 0;		/* maybe a lie? */
	dev->needs_freset = 0;		/* pcie fundamental reset required */
	set_pcie_port_type(dev);

	list_for_each_entry(slot, &dev->bus->slots, list)
		if (PCI_SLOT(dev->devfn) == slot->number)
			dev->slot = slot;

	dev->vendor = get_int_prop(node, "vendor-id", 0xffff);
	dev->device = get_int_prop(node, "device-id", 0xffff);
	dev->subsystem_vendor = get_int_prop(node, "subsystem-vendor-id", 0);
	dev->subsystem_device = get_int_prop(node, "subsystem-id", 0);

	dev->cfg_size = pci_cfg_space_size(dev);

	dev_set_name(&dev->dev, "%04x:%02x:%02x.%d", pci_domain_nr(bus),
		dev->bus->number, PCI_SLOT(devfn), PCI_FUNC(devfn));
	dev->class = get_int_prop(node, "class-code", 0);
	dev->revision = get_int_prop(node, "revision-id", 0);

	pr_debug("    class: 0x%x\n", dev->class);
	pr_debug("    revision: 0x%x\n", dev->revision);

	dev->current_state = 4;		/* unknown power state */
	dev->error_state = pci_channel_io_normal;
	dev->dma_mask = 0xffffffff;

	/* Early fixups, before probing the BARs */
	pci_fixup_device(pci_fixup_early, dev);

	if (!strcmp(type, "pci") || !strcmp(type, "pciex")) {
		/* a PCI-PCI bridge */
		dev->hdr_type = PCI_HEADER_TYPE_BRIDGE;
		dev->rom_base_reg = PCI_ROM_ADDRESS1;
		set_pcie_hotplug_bridge(dev);
	} else if (!strcmp(type, "cardbus")) {
		dev->hdr_type = PCI_HEADER_TYPE_CARDBUS;
	} else {
		dev->hdr_type = PCI_HEADER_TYPE_NORMAL;
		dev->rom_base_reg = PCI_ROM_ADDRESS;
		/* Maybe do a default OF mapping here */
		dev->irq = NO_IRQ;
	}

	of_pci_parse_addrs(node, dev);

	pr_debug("    adding to system ...\n");

	pci_device_add(dev, bus);

	return dev;
}
EXPORT_SYMBOL(of_create_pci_dev);

void __devinit of_scan_pci_bridge(struct device_node *node,
				  struct pci_dev *dev)
{
	struct pci_bus *bus;
	const u32 *busrange, *ranges;
	int len, i, mode;
	struct resource *res;
	unsigned int flags;
	u64 size;

	pr_debug("of_scan_pci_bridge(%s)\n", node->full_name);

	/* parse bus-range property */
	busrange = of_get_property(node, "bus-range", &len);
	if (busrange == NULL || len != 8) {
		printk(KERN_DEBUG "Can't get bus-range for PCI-PCI bridge %s\n",
		       node->full_name);
		return;
	}
	ranges = of_get_property(node, "ranges", &len);
	if (ranges == NULL) {
		printk(KERN_DEBUG "Can't get ranges for PCI-PCI bridge %s\n",
		       node->full_name);
		return;
	}

	bus = pci_add_new_bus(dev->bus, dev, busrange[0]);
	if (!bus) {
		printk(KERN_ERR "Failed to create pci bus for %s\n",
		       node->full_name);
		return;
	}

	bus->primary = dev->bus->number;
	bus->subordinate = busrange[1];
	bus->bridge_ctl = 0;
	bus->sysdata = node;

	/* parse ranges property */
	/* PCI #address-cells == 3 and #size-cells == 2 always */
	res = &dev->resource[PCI_BRIDGE_RESOURCES];
	for (i = 0; i < PCI_NUM_RESOURCES - PCI_BRIDGE_RESOURCES; ++i) {
		res->flags = 0;
		bus->resource[i] = res;
		++res;
	}
	i = 1;
	for (; len >= 32; len -= 32, ranges += 8) {
		flags = pci_parse_of_flags(ranges[0], 1);
		size = of_read_number(&ranges[6], 2);
		if (flags == 0 || size == 0)
			continue;
		if (flags & IORESOURCE_IO) {
			res = bus->resource[0];
			if (res->flags) {
				printk(KERN_ERR "PCI: ignoring extra I/O range"
				       " for bridge %s\n", node->full_name);
				continue;
			}
		} else {
			if (i >= PCI_NUM_RESOURCES - PCI_BRIDGE_RESOURCES) {
				printk(KERN_ERR "PCI: too many memory ranges"
				       " for bridge %s\n", node->full_name);
				continue;
			}
			res = bus->resource[i];
			++i;
		}
		res->start = of_read_number(&ranges[1], 2);
		res->end = res->start + size - 1;
		res->flags = flags;
	}
	sprintf(bus->name, "PCI Bus %04x:%02x", pci_domain_nr(bus),
		bus->number);
	pr_debug("    bus name: %s\n", bus->name);

	mode = PCI_PROBE_NORMAL;
	if (ppc_md.pci_probe_mode)
		mode = ppc_md.pci_probe_mode(bus);
	pr_debug("    probe mode: %d\n", mode);

	if (mode == PCI_PROBE_DEVTREE)
		of_scan_bus(node, bus);
	else if (mode == PCI_PROBE_NORMAL)
		pci_scan_child_bus(bus);
}
EXPORT_SYMBOL(of_scan_pci_bridge);

static void __devinit __of_scan_bus(struct device_node *node,
				    struct pci_bus *bus, int rescan_existing)
{
	struct device_node *child;
	const u32 *reg;
	int reglen, devfn;
	struct pci_dev *dev;

	pr_debug("of_scan_bus(%s) bus no %d...\n",
		 node->full_name, bus->number);

	/* Scan direct children */
	for_each_child_of_node(node, child) {
		pr_debug("  * %s\n", child->full_name);
		if (!of_device_is_available(child))
			continue;
		reg = of_get_property(child, "reg", &reglen);
		if (reg == NULL || reglen < 20)
			continue;
		devfn = (reg[0] >> 8) & 0xff;

		/* create a new pci_dev for this device */
		dev = of_create_pci_dev(child, bus, devfn);
		if (!dev)
			continue;
		pr_debug("    dev header type: %x\n", dev->hdr_type);
	}

	/* Apply all fixups necessary. We don't fixup the bus "self"
	 * for an existing bridge that is being rescanned
	 */
	if (!rescan_existing)
		pcibios_setup_bus_self(bus);
	pcibios_setup_bus_devices(bus);

	/* Now scan child busses */
	list_for_each_entry(dev, &bus->devices, bus_list) {
		if (dev->hdr_type == PCI_HEADER_TYPE_BRIDGE ||
		    dev->hdr_type == PCI_HEADER_TYPE_CARDBUS) {
			struct device_node *child = pci_device_to_OF_node(dev);
			if (dev)
				of_scan_pci_bridge(child, dev);
		}
	}
}

void __devinit of_scan_bus(struct device_node *node,
			   struct pci_bus *bus)
{
	__of_scan_bus(node, bus, 0);
}
EXPORT_SYMBOL_GPL(of_scan_bus);

void __devinit of_rescan_bus(struct device_node *node,
			     struct pci_bus *bus)
{
	__of_scan_bus(node, bus, 1);
}
EXPORT_SYMBOL_GPL(of_rescan_bus);

