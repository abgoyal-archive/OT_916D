

#include <linux/signal.h>

#include <linux/of.h>
#include <linux/of_platform.h>

static int ehci_xilinx_of_setup(struct usb_hcd *hcd)
{
	struct ehci_hcd	*ehci = hcd_to_ehci(hcd);
	int		retval;

	retval = ehci_halt(ehci);
	if (retval)
		return retval;

	retval = ehci_init(hcd);
	if (retval)
		return retval;

	ehci->sbrn = 0x20;

	return ehci_reset(ehci);
}

static int ehci_xilinx_port_handed_over(struct usb_hcd *hcd, int portnum)
{
	dev_warn(hcd->self.controller, "port %d cannot be enabled\n", portnum);
	if (hcd->has_tt) {
		dev_warn(hcd->self.controller,
			"Maybe you have connected a low speed device?\n");

		dev_warn(hcd->self.controller,
			"We do not support low speed devices\n");
	} else {
		dev_warn(hcd->self.controller,
			"Maybe your device is not a high speed device?\n");
		dev_warn(hcd->self.controller,
			"The USB host controller does not support full speed "
			"nor low speed devices\n");
		dev_warn(hcd->self.controller,
			"You can reconfigure the host controller to have "
			"full speed support\n");
	}

	return 0;
}


static const struct hc_driver ehci_xilinx_of_hc_driver = {
	.description		= hcd_name,
	.product_desc		= "OF EHCI",
	.hcd_priv_size		= sizeof(struct ehci_hcd),

	/*
	 * generic hardware linkage
	 */
	.irq			= ehci_irq,
	.flags			= HCD_MEMORY | HCD_USB2,

	/*
	 * basic lifecycle operations
	 */
	.reset			= ehci_xilinx_of_setup,
	.start			= ehci_run,
	.stop			= ehci_stop,
	.shutdown		= ehci_shutdown,

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue		= ehci_urb_enqueue,
	.urb_dequeue		= ehci_urb_dequeue,
	.endpoint_disable	= ehci_endpoint_disable,

	/*
	 * scheduling support
	 */
	.get_frame_number	= ehci_get_frame,

	/*
	 * root hub support
	 */
	.hub_status_data	= ehci_hub_status_data,
	.hub_control		= ehci_hub_control,
#ifdef	CONFIG_PM
	.bus_suspend		= ehci_bus_suspend,
	.bus_resume		= ehci_bus_resume,
#endif
	.relinquish_port	= NULL,
	.port_handed_over	= ehci_xilinx_port_handed_over,

	.clear_tt_buffer_complete = ehci_clear_tt_buffer_complete,
};

static int __devinit
ehci_hcd_xilinx_of_probe(struct of_device *op, const struct of_device_id *match)
{
	struct device_node *dn = op->dev.of_node;
	struct usb_hcd *hcd;
	struct ehci_hcd	*ehci;
	struct resource res;
	int irq;
	int rv;
	int *value;

	if (usb_disabled())
		return -ENODEV;

	dev_dbg(&op->dev, "initializing XILINX-OF USB Controller\n");

	rv = of_address_to_resource(dn, 0, &res);
	if (rv)
		return rv;

	hcd = usb_create_hcd(&ehci_xilinx_of_hc_driver, &op->dev,
				"XILINX-OF USB");
	if (!hcd)
		return -ENOMEM;

	hcd->rsrc_start = res.start;
	hcd->rsrc_len = res.end - res.start + 1;

	if (!request_mem_region(hcd->rsrc_start, hcd->rsrc_len, hcd_name)) {
		printk(KERN_ERR "%s: request_mem_region failed\n", __FILE__);
		rv = -EBUSY;
		goto err_rmr;
	}

	irq = irq_of_parse_and_map(dn, 0);
	if (irq == NO_IRQ) {
		printk(KERN_ERR "%s: irq_of_parse_and_map failed\n", __FILE__);
		rv = -EBUSY;
		goto err_irq;
	}

	hcd->regs = ioremap(hcd->rsrc_start, hcd->rsrc_len);
	if (!hcd->regs) {
		printk(KERN_ERR "%s: ioremap failed\n", __FILE__);
		rv = -ENOMEM;
		goto err_ioremap;
	}

	ehci = hcd_to_ehci(hcd);

	/* This core always has big-endian register interface and uses
	 * big-endian memory descriptors.
	 */
	ehci->big_endian_mmio = 1;
	ehci->big_endian_desc = 1;

	/* Check whether the FS support option is selected in the hardware.
	 */
	value = (int *)of_get_property(dn, "xlnx,support-usb-fs", NULL);
	if (value && (*value == 1)) {
		ehci_dbg(ehci, "USB host controller supports FS devices\n");
		hcd->has_tt = 1;
	} else {
		ehci_dbg(ehci,
			"USB host controller is HS only\n");
		hcd->has_tt = 0;
	}

	/* Debug registers are at the first 0x100 region
	 */
	ehci->caps = hcd->regs + 0x100;
	ehci->regs = hcd->regs + 0x100 +
			HC_LENGTH(ehci_readl(ehci, &ehci->caps->hc_capbase));

	/* cache this readonly data; minimize chip reads */
	ehci->hcs_params = ehci_readl(ehci, &ehci->caps->hcs_params);

	rv = usb_add_hcd(hcd, irq, 0);
	if (rv == 0)
		return 0;

	iounmap(hcd->regs);

err_ioremap:
err_irq:
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
err_rmr:
	usb_put_hcd(hcd);

	return rv;
}

static int ehci_hcd_xilinx_of_remove(struct of_device *op)
{
	struct usb_hcd *hcd = dev_get_drvdata(&op->dev);
	dev_set_drvdata(&op->dev, NULL);

	dev_dbg(&op->dev, "stopping XILINX-OF USB Controller\n");

	usb_remove_hcd(hcd);

	iounmap(hcd->regs);
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);

	usb_put_hcd(hcd);

	return 0;
}

static int ehci_hcd_xilinx_of_shutdown(struct of_device *op)
{
	struct usb_hcd *hcd = dev_get_drvdata(&op->dev);

	if (hcd->driver->shutdown)
		hcd->driver->shutdown(hcd);

	return 0;
}


static const struct of_device_id ehci_hcd_xilinx_of_match[] = {
		{.compatible = "xlnx,xps-usb-host-1.00.a",},
	{},
};
MODULE_DEVICE_TABLE(of, ehci_hcd_xilinx_of_match);

static struct of_platform_driver ehci_hcd_xilinx_of_driver = {
	.probe		= ehci_hcd_xilinx_of_probe,
	.remove		= ehci_hcd_xilinx_of_remove,
	.shutdown	= ehci_hcd_xilinx_of_shutdown,
	.driver = {
		.name = "xilinx-of-ehci",
		.owner = THIS_MODULE,
		.of_match_table = ehci_hcd_xilinx_of_match,
	},
};
