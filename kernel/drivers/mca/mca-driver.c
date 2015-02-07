
/* -*- mode: c; c-basic-offset: 8 -*- */


#include <linux/device.h>
#include <linux/mca.h>
#include <linux/module.h>

int mca_register_driver(struct mca_driver *mca_drv)
{
	int r;

	if (MCA_bus) {
		mca_drv->driver.bus = &mca_bus_type;
		if ((r = driver_register(&mca_drv->driver)) < 0)
			return r;
		mca_drv->integrated_id = 0;
	}

	return 0;
}
EXPORT_SYMBOL(mca_register_driver);

int mca_register_driver_integrated(struct mca_driver *mca_driver,
				   int integrated_id)
{
	int r = mca_register_driver(mca_driver);

	if (!r)
		mca_driver->integrated_id = integrated_id;

	return r;
}
EXPORT_SYMBOL(mca_register_driver_integrated);

void mca_unregister_driver(struct mca_driver *mca_drv)
{
	if (MCA_bus)
		driver_unregister(&mca_drv->driver);
}
EXPORT_SYMBOL(mca_unregister_driver);
