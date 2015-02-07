

#include <linux/types.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/console.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/dma-mapping.h>
#include <linux/kobject.h>

#include <asm/iommu.h>
#include <asm/dma.h>
#include <asm/vio.h>
#include <asm/prom.h>
#include <asm/firmware.h>
#include <asm/tce.h>
#include <asm/abs_addr.h>
#include <asm/page.h>
#include <asm/hvcall.h>
#include <asm/iseries/vio.h>
#include <asm/iseries/hv_types.h>
#include <asm/iseries/hv_lp_config.h>
#include <asm/iseries/hv_call_xm.h>
#include <asm/iseries/iommu.h>

static struct bus_type vio_bus_type;

static struct vio_dev vio_bus_device  = { /* fake "parent" device */
	.name = "vio",
	.type = "",
	.dev.init_name = "vio",
	.dev.bus = &vio_bus_type,
};

#ifdef CONFIG_PPC_SMLPAR
struct vio_cmo_pool {
	size_t size;
	size_t free;
};

/* How many ms to delay queued balance work */
#define VIO_CMO_BALANCE_DELAY 100

/* Portion out IO memory to CMO devices by this chunk size */
#define VIO_CMO_BALANCE_CHUNK 131072

struct vio_cmo_dev_entry {
	struct vio_dev *viodev;
	struct list_head list;
};

struct vio_cmo {
	spinlock_t lock;
	struct delayed_work balance_q;
	struct list_head device_list;
	size_t entitled;
	struct vio_cmo_pool reserve;
	struct vio_cmo_pool excess;
	size_t spare;
	size_t min;
	size_t desired;
	size_t curr;
	size_t high;
} vio_cmo;

static int vio_cmo_num_OF_devs(void)
{
	struct device_node *node_vroot;
	int count = 0;

	/*
	 * Count the number of vdevice entries with an
	 * ibm,my-dma-window OF property
	 */
	node_vroot = of_find_node_by_name(NULL, "vdevice");
	if (node_vroot) {
		struct device_node *of_node;
		struct property *prop;

		for_each_child_of_node(node_vroot, of_node) {
			prop = of_find_property(of_node, "ibm,my-dma-window",
			                       NULL);
			if (prop)
				count++;
		}
	}
	of_node_put(node_vroot);
	return count;
}

static inline int vio_cmo_alloc(struct vio_dev *viodev, size_t size)
{
	unsigned long flags;
	size_t reserve_free = 0;
	size_t excess_free = 0;
	int ret = -ENOMEM;

	spin_lock_irqsave(&vio_cmo.lock, flags);

	/* Determine the amount of free entitlement available in reserve */
	if (viodev->cmo.entitled > viodev->cmo.allocated)
		reserve_free = viodev->cmo.entitled - viodev->cmo.allocated;

	/* If spare is not fulfilled, the excess pool can not be used. */
	if (vio_cmo.spare >= VIO_CMO_MIN_ENT)
		excess_free = vio_cmo.excess.free;

	/* The request can be satisfied */
	if ((reserve_free + excess_free) >= size) {
		vio_cmo.curr += size;
		if (vio_cmo.curr > vio_cmo.high)
			vio_cmo.high = vio_cmo.curr;
		viodev->cmo.allocated += size;
		size -= min(reserve_free, size);
		vio_cmo.excess.free -= size;
		ret = 0;
	}

	spin_unlock_irqrestore(&vio_cmo.lock, flags);
	return ret;
}

static inline void vio_cmo_dealloc(struct vio_dev *viodev, size_t size)
{
	unsigned long flags;
	size_t spare_needed = 0;
	size_t excess_freed = 0;
	size_t reserve_freed = size;
	size_t tmp;
	int balance = 0;

	spin_lock_irqsave(&vio_cmo.lock, flags);
	vio_cmo.curr -= size;

	/* Amount of memory freed from the excess pool */
	if (viodev->cmo.allocated > viodev->cmo.entitled) {
		excess_freed = min(reserve_freed, (viodev->cmo.allocated -
		                                   viodev->cmo.entitled));
		reserve_freed -= excess_freed;
	}

	/* Remove allocation from device */
	viodev->cmo.allocated -= (reserve_freed + excess_freed);

	/* Spare is a subset of the reserve pool, replenish it first. */
	spare_needed = VIO_CMO_MIN_ENT - vio_cmo.spare;

	/*
	 * Replenish the spare in the reserve pool from the excess pool.
	 * This moves entitlement into the reserve pool.
	 */
	if (spare_needed && excess_freed) {
		tmp = min(excess_freed, spare_needed);
		vio_cmo.excess.size -= tmp;
		vio_cmo.reserve.size += tmp;
		vio_cmo.spare += tmp;
		excess_freed -= tmp;
		spare_needed -= tmp;
		balance = 1;
	}

	/*
	 * Replenish the spare in the reserve pool from the reserve pool.
	 * This removes entitlement from the device down to VIO_CMO_MIN_ENT,
	 * if needed, and gives it to the spare pool. The amount of used
	 * memory in this pool does not change.
	 */
	if (spare_needed && reserve_freed) {
		tmp = min(spare_needed, min(reserve_freed,
		                            (viodev->cmo.entitled -
		                             VIO_CMO_MIN_ENT)));

		vio_cmo.spare += tmp;
		viodev->cmo.entitled -= tmp;
		reserve_freed -= tmp;
		spare_needed -= tmp;
		balance = 1;
	}

	/*
	 * Increase the reserve pool until the desired allocation is met.
	 * Move an allocation freed from the excess pool into the reserve
	 * pool and schedule a balance operation.
	 */
	if (excess_freed && (vio_cmo.desired > vio_cmo.reserve.size)) {
		tmp = min(excess_freed, (vio_cmo.desired - vio_cmo.reserve.size));

		vio_cmo.excess.size -= tmp;
		vio_cmo.reserve.size += tmp;
		excess_freed -= tmp;
		balance = 1;
	}

	/* Return memory from the excess pool to that pool */
	if (excess_freed)
		vio_cmo.excess.free += excess_freed;

	if (balance)
		schedule_delayed_work(&vio_cmo.balance_q, VIO_CMO_BALANCE_DELAY);
	spin_unlock_irqrestore(&vio_cmo.lock, flags);
}

int vio_cmo_entitlement_update(size_t new_entitlement)
{
	struct vio_dev *viodev;
	struct vio_cmo_dev_entry *dev_ent;
	unsigned long flags;
	size_t avail, delta, tmp;

	spin_lock_irqsave(&vio_cmo.lock, flags);

	/* Entitlement increases */
	if (new_entitlement > vio_cmo.entitled) {
		delta = new_entitlement - vio_cmo.entitled;

		/* Fulfill spare allocation */
		if (vio_cmo.spare < VIO_CMO_MIN_ENT) {
			tmp = min(delta, (VIO_CMO_MIN_ENT - vio_cmo.spare));
			vio_cmo.spare += tmp;
			vio_cmo.reserve.size += tmp;
			delta -= tmp;
		}

		/* Remaining new allocation goes to the excess pool */
		vio_cmo.entitled += delta;
		vio_cmo.excess.size += delta;
		vio_cmo.excess.free += delta;

		goto out;
	}

	/* Entitlement decreases */
	delta = vio_cmo.entitled - new_entitlement;
	avail = vio_cmo.excess.free;

	/*
	 * Need to check how much unused entitlement each device can
	 * sacrifice to fulfill entitlement change.
	 */
	list_for_each_entry(dev_ent, &vio_cmo.device_list, list) {
		if (avail >= delta)
			break;

		viodev = dev_ent->viodev;
		if ((viodev->cmo.entitled > viodev->cmo.allocated) &&
		    (viodev->cmo.entitled > VIO_CMO_MIN_ENT))
				avail += viodev->cmo.entitled -
				         max_t(size_t, viodev->cmo.allocated,
				               VIO_CMO_MIN_ENT);
	}

	if (delta <= avail) {
		vio_cmo.entitled -= delta;

		/* Take entitlement from the excess pool first */
		tmp = min(vio_cmo.excess.free, delta);
		vio_cmo.excess.size -= tmp;
		vio_cmo.excess.free -= tmp;
		delta -= tmp;

		/*
		 * Remove all but VIO_CMO_MIN_ENT bytes from devices
		 * until entitlement change is served
		 */
		list_for_each_entry(dev_ent, &vio_cmo.device_list, list) {
			if (!delta)
				break;

			viodev = dev_ent->viodev;
			tmp = 0;
			if ((viodev->cmo.entitled > viodev->cmo.allocated) &&
			    (viodev->cmo.entitled > VIO_CMO_MIN_ENT))
				tmp = viodev->cmo.entitled -
				      max_t(size_t, viodev->cmo.allocated,
				            VIO_CMO_MIN_ENT);
			viodev->cmo.entitled -= min(tmp, delta);
			delta -= min(tmp, delta);
		}
	} else {
		spin_unlock_irqrestore(&vio_cmo.lock, flags);
		return -ENOMEM;
	}

out:
	schedule_delayed_work(&vio_cmo.balance_q, 0);
	spin_unlock_irqrestore(&vio_cmo.lock, flags);
	return 0;
}

static void vio_cmo_balance(struct work_struct *work)
{
	struct vio_cmo *cmo;
	struct vio_dev *viodev;
	struct vio_cmo_dev_entry *dev_ent;
	unsigned long flags;
	size_t avail = 0, level, chunk, need;
	int devcount = 0, fulfilled;

	cmo = container_of(work, struct vio_cmo, balance_q.work);

	spin_lock_irqsave(&vio_cmo.lock, flags);

	/* Calculate minimum entitlement and fulfill spare */
	cmo->min = vio_cmo_num_OF_devs() * VIO_CMO_MIN_ENT;
	BUG_ON(cmo->min > cmo->entitled);
	cmo->spare = min_t(size_t, VIO_CMO_MIN_ENT, (cmo->entitled - cmo->min));
	cmo->min += cmo->spare;
	cmo->desired = cmo->min;

	/*
	 * Determine how much entitlement is available and reset device
	 * entitlements
	 */
	avail = cmo->entitled - cmo->spare;
	list_for_each_entry(dev_ent, &vio_cmo.device_list, list) {
		viodev = dev_ent->viodev;
		devcount++;
		viodev->cmo.entitled = VIO_CMO_MIN_ENT;
		cmo->desired += (viodev->cmo.desired - VIO_CMO_MIN_ENT);
		avail -= max_t(size_t, viodev->cmo.allocated, VIO_CMO_MIN_ENT);
	}

	/*
	 * Having provided each device with the minimum entitlement, loop
	 * over the devices portioning out the remaining entitlement
	 * until there is nothing left.
	 */
	level = VIO_CMO_MIN_ENT;
	while (avail) {
		fulfilled = 0;
		list_for_each_entry(dev_ent, &vio_cmo.device_list, list) {
			viodev = dev_ent->viodev;

			if (viodev->cmo.desired <= level) {
				fulfilled++;
				continue;
			}

			/*
			 * Give the device up to VIO_CMO_BALANCE_CHUNK
			 * bytes of entitlement, but do not exceed the
			 * desired level of entitlement for the device.
			 */
			chunk = min_t(size_t, avail, VIO_CMO_BALANCE_CHUNK);
			chunk = min(chunk, (viodev->cmo.desired -
			                    viodev->cmo.entitled));
			viodev->cmo.entitled += chunk;

			/*
			 * If the memory for this entitlement increase was
			 * already allocated to the device it does not come
			 * from the available pool being portioned out.
			 */
			need = max(viodev->cmo.allocated, viodev->cmo.entitled)-
			       max(viodev->cmo.allocated, level);
			avail -= need;

		}
		if (fulfilled == devcount)
			break;
		level += VIO_CMO_BALANCE_CHUNK;
	}

	/* Calculate new reserve and excess pool sizes */
	cmo->reserve.size = cmo->min;
	cmo->excess.free = 0;
	cmo->excess.size = 0;
	need = 0;
	list_for_each_entry(dev_ent, &vio_cmo.device_list, list) {
		viodev = dev_ent->viodev;
		/* Calculated reserve size above the minimum entitlement */
		if (viodev->cmo.entitled)
			cmo->reserve.size += (viodev->cmo.entitled -
			                      VIO_CMO_MIN_ENT);
		/* Calculated used excess entitlement */
		if (viodev->cmo.allocated > viodev->cmo.entitled)
			need += viodev->cmo.allocated - viodev->cmo.entitled;
	}
	cmo->excess.size = cmo->entitled - cmo->reserve.size;
	cmo->excess.free = cmo->excess.size - need;

	cancel_delayed_work(to_delayed_work(work));
	spin_unlock_irqrestore(&vio_cmo.lock, flags);
}

static void *vio_dma_iommu_alloc_coherent(struct device *dev, size_t size,
                                          dma_addr_t *dma_handle, gfp_t flag)
{
	struct vio_dev *viodev = to_vio_dev(dev);
	void *ret;

	if (vio_cmo_alloc(viodev, roundup(size, PAGE_SIZE))) {
		atomic_inc(&viodev->cmo.allocs_failed);
		return NULL;
	}

	ret = dma_iommu_ops.alloc_coherent(dev, size, dma_handle, flag);
	if (unlikely(ret == NULL)) {
		vio_cmo_dealloc(viodev, roundup(size, PAGE_SIZE));
		atomic_inc(&viodev->cmo.allocs_failed);
	}

	return ret;
}

static void vio_dma_iommu_free_coherent(struct device *dev, size_t size,
                                        void *vaddr, dma_addr_t dma_handle)
{
	struct vio_dev *viodev = to_vio_dev(dev);

	dma_iommu_ops.free_coherent(dev, size, vaddr, dma_handle);

	vio_cmo_dealloc(viodev, roundup(size, PAGE_SIZE));
}

static dma_addr_t vio_dma_iommu_map_page(struct device *dev, struct page *page,
                                         unsigned long offset, size_t size,
                                         enum dma_data_direction direction,
                                         struct dma_attrs *attrs)
{
	struct vio_dev *viodev = to_vio_dev(dev);
	dma_addr_t ret = DMA_ERROR_CODE;

	if (vio_cmo_alloc(viodev, roundup(size, IOMMU_PAGE_SIZE))) {
		atomic_inc(&viodev->cmo.allocs_failed);
		return ret;
	}

	ret = dma_iommu_ops.map_page(dev, page, offset, size, direction, attrs);
	if (unlikely(dma_mapping_error(dev, ret))) {
		vio_cmo_dealloc(viodev, roundup(size, IOMMU_PAGE_SIZE));
		atomic_inc(&viodev->cmo.allocs_failed);
	}

	return ret;
}

static void vio_dma_iommu_unmap_page(struct device *dev, dma_addr_t dma_handle,
				     size_t size,
				     enum dma_data_direction direction,
				     struct dma_attrs *attrs)
{
	struct vio_dev *viodev = to_vio_dev(dev);

	dma_iommu_ops.unmap_page(dev, dma_handle, size, direction, attrs);

	vio_cmo_dealloc(viodev, roundup(size, IOMMU_PAGE_SIZE));
}

static int vio_dma_iommu_map_sg(struct device *dev, struct scatterlist *sglist,
                                int nelems, enum dma_data_direction direction,
                                struct dma_attrs *attrs)
{
	struct vio_dev *viodev = to_vio_dev(dev);
	struct scatterlist *sgl;
	int ret, count = 0;
	size_t alloc_size = 0;

	for (sgl = sglist; count < nelems; count++, sgl++)
		alloc_size += roundup(sgl->length, IOMMU_PAGE_SIZE);

	if (vio_cmo_alloc(viodev, alloc_size)) {
		atomic_inc(&viodev->cmo.allocs_failed);
		return 0;
	}

	ret = dma_iommu_ops.map_sg(dev, sglist, nelems, direction, attrs);

	if (unlikely(!ret)) {
		vio_cmo_dealloc(viodev, alloc_size);
		atomic_inc(&viodev->cmo.allocs_failed);
		return ret;
	}

	for (sgl = sglist, count = 0; count < ret; count++, sgl++)
		alloc_size -= roundup(sgl->dma_length, IOMMU_PAGE_SIZE);
	if (alloc_size)
		vio_cmo_dealloc(viodev, alloc_size);

	return ret;
}

static void vio_dma_iommu_unmap_sg(struct device *dev,
		struct scatterlist *sglist, int nelems,
		enum dma_data_direction direction,
		struct dma_attrs *attrs)
{
	struct vio_dev *viodev = to_vio_dev(dev);
	struct scatterlist *sgl;
	size_t alloc_size = 0;
	int count = 0;

	for (sgl = sglist; count < nelems; count++, sgl++)
		alloc_size += roundup(sgl->dma_length, IOMMU_PAGE_SIZE);

	dma_iommu_ops.unmap_sg(dev, sglist, nelems, direction, attrs);

	vio_cmo_dealloc(viodev, alloc_size);
}

struct dma_map_ops vio_dma_mapping_ops = {
	.alloc_coherent = vio_dma_iommu_alloc_coherent,
	.free_coherent  = vio_dma_iommu_free_coherent,
	.map_sg         = vio_dma_iommu_map_sg,
	.unmap_sg       = vio_dma_iommu_unmap_sg,
	.map_page       = vio_dma_iommu_map_page,
	.unmap_page     = vio_dma_iommu_unmap_page,

};

void vio_cmo_set_dev_desired(struct vio_dev *viodev, size_t desired)
{
	unsigned long flags;
	struct vio_cmo_dev_entry *dev_ent;
	int found = 0;

	if (!firmware_has_feature(FW_FEATURE_CMO))
		return;

	spin_lock_irqsave(&vio_cmo.lock, flags);
	if (desired < VIO_CMO_MIN_ENT)
		desired = VIO_CMO_MIN_ENT;

	/*
	 * Changes will not be made for devices not in the device list.
	 * If it is not in the device list, then no driver is loaded
	 * for the device and it can not receive entitlement.
	 */
	list_for_each_entry(dev_ent, &vio_cmo.device_list, list)
		if (viodev == dev_ent->viodev) {
			found = 1;
			break;
		}
	if (!found) {
		spin_unlock_irqrestore(&vio_cmo.lock, flags);
		return;
	}

	/* Increase/decrease in desired device entitlement */
	if (desired >= viodev->cmo.desired) {
		/* Just bump the bus and device values prior to a balance*/
		vio_cmo.desired += desired - viodev->cmo.desired;
		viodev->cmo.desired = desired;
	} else {
		/* Decrease bus and device values for desired entitlement */
		vio_cmo.desired -= viodev->cmo.desired - desired;
		viodev->cmo.desired = desired;
		/*
		 * If less entitlement is desired than current entitlement, move
		 * any reserve memory in the change region to the excess pool.
		 */
		if (viodev->cmo.entitled > desired) {
			vio_cmo.reserve.size -= viodev->cmo.entitled - desired;
			vio_cmo.excess.size += viodev->cmo.entitled - desired;
			/*
			 * If entitlement moving from the reserve pool to the
			 * excess pool is currently unused, add to the excess
			 * free counter.
			 */
			if (viodev->cmo.allocated < viodev->cmo.entitled)
				vio_cmo.excess.free += viodev->cmo.entitled -
				                       max(viodev->cmo.allocated, desired);
			viodev->cmo.entitled = desired;
		}
	}
	schedule_delayed_work(&vio_cmo.balance_q, 0);
	spin_unlock_irqrestore(&vio_cmo.lock, flags);
}

static int vio_cmo_bus_probe(struct vio_dev *viodev)
{
	struct vio_cmo_dev_entry *dev_ent;
	struct device *dev = &viodev->dev;
	struct vio_driver *viodrv = to_vio_driver(dev->driver);
	unsigned long flags;
	size_t size;

	/*
	 * Check to see that device has a DMA window and configure
	 * entitlement for the device.
	 */
	if (of_get_property(viodev->dev.of_node,
	                    "ibm,my-dma-window", NULL)) {
		/* Check that the driver is CMO enabled and get desired DMA */
		if (!viodrv->get_desired_dma) {
			dev_err(dev, "%s: device driver does not support CMO\n",
			        __func__);
			return -EINVAL;
		}

		viodev->cmo.desired = IOMMU_PAGE_ALIGN(viodrv->get_desired_dma(viodev));
		if (viodev->cmo.desired < VIO_CMO_MIN_ENT)
			viodev->cmo.desired = VIO_CMO_MIN_ENT;
		size = VIO_CMO_MIN_ENT;

		dev_ent = kmalloc(sizeof(struct vio_cmo_dev_entry),
		                  GFP_KERNEL);
		if (!dev_ent)
			return -ENOMEM;

		dev_ent->viodev = viodev;
		spin_lock_irqsave(&vio_cmo.lock, flags);
		list_add(&dev_ent->list, &vio_cmo.device_list);
	} else {
		viodev->cmo.desired = 0;
		size = 0;
		spin_lock_irqsave(&vio_cmo.lock, flags);
	}

	/*
	 * If the needs for vio_cmo.min have not changed since they
	 * were last set, the number of devices in the OF tree has
	 * been constant and the IO memory for this is already in
	 * the reserve pool.
	 */
	if (vio_cmo.min == ((vio_cmo_num_OF_devs() + 1) *
	                    VIO_CMO_MIN_ENT)) {
		/* Updated desired entitlement if device requires it */
		if (size)
			vio_cmo.desired += (viodev->cmo.desired -
		                        VIO_CMO_MIN_ENT);
	} else {
		size_t tmp;

		tmp = vio_cmo.spare + vio_cmo.excess.free;
		if (tmp < size) {
			dev_err(dev, "%s: insufficient free "
			        "entitlement to add device. "
			        "Need %lu, have %lu\n", __func__,
				size, (vio_cmo.spare + tmp));
			spin_unlock_irqrestore(&vio_cmo.lock, flags);
			return -ENOMEM;
		}

		/* Use excess pool first to fulfill request */
		tmp = min(size, vio_cmo.excess.free);
		vio_cmo.excess.free -= tmp;
		vio_cmo.excess.size -= tmp;
		vio_cmo.reserve.size += tmp;

		/* Use spare if excess pool was insufficient */
		vio_cmo.spare -= size - tmp;

		/* Update bus accounting */
		vio_cmo.min += size;
		vio_cmo.desired += viodev->cmo.desired;
	}
	spin_unlock_irqrestore(&vio_cmo.lock, flags);
	return 0;
}

static void vio_cmo_bus_remove(struct vio_dev *viodev)
{
	struct vio_cmo_dev_entry *dev_ent;
	unsigned long flags;
	size_t tmp;

	spin_lock_irqsave(&vio_cmo.lock, flags);
	if (viodev->cmo.allocated) {
		dev_err(&viodev->dev, "%s: device had %lu bytes of IO "
		        "allocated after remove operation.\n",
		        __func__, viodev->cmo.allocated);
		BUG();
	}

	/*
	 * Remove the device from the device list being maintained for
	 * CMO enabled devices.
	 */
	list_for_each_entry(dev_ent, &vio_cmo.device_list, list)
		if (viodev == dev_ent->viodev) {
			list_del(&dev_ent->list);
			kfree(dev_ent);
			break;
		}

	/*
	 * Devices may not require any entitlement and they do not need
	 * to be processed.  Otherwise, return the device's entitlement
	 * back to the pools.
	 */
	if (viodev->cmo.entitled) {
		/*
		 * This device has not yet left the OF tree, it's
		 * minimum entitlement remains in vio_cmo.min and
		 * vio_cmo.desired
		 */
		vio_cmo.desired -= (viodev->cmo.desired - VIO_CMO_MIN_ENT);

		/*
		 * Save min allocation for device in reserve as long
		 * as it exists in OF tree as determined by later
		 * balance operation
		 */
		viodev->cmo.entitled -= VIO_CMO_MIN_ENT;

		/* Replenish spare from freed reserve pool */
		if (viodev->cmo.entitled && (vio_cmo.spare < VIO_CMO_MIN_ENT)) {
			tmp = min(viodev->cmo.entitled, (VIO_CMO_MIN_ENT -
			                                 vio_cmo.spare));
			vio_cmo.spare += tmp;
			viodev->cmo.entitled -= tmp;
		}

		/* Remaining reserve goes to excess pool */
		vio_cmo.excess.size += viodev->cmo.entitled;
		vio_cmo.excess.free += viodev->cmo.entitled;
		vio_cmo.reserve.size -= viodev->cmo.entitled;

		/*
		 * Until the device is removed it will keep a
		 * minimum entitlement; this will guarantee that
		 * a module unload/load will result in a success.
		 */
		viodev->cmo.entitled = VIO_CMO_MIN_ENT;
		viodev->cmo.desired = VIO_CMO_MIN_ENT;
		atomic_set(&viodev->cmo.allocs_failed, 0);
	}

	spin_unlock_irqrestore(&vio_cmo.lock, flags);
}

static void vio_cmo_set_dma_ops(struct vio_dev *viodev)
{
	vio_dma_mapping_ops.dma_supported = dma_iommu_ops.dma_supported;
	viodev->dev.archdata.dma_ops = &vio_dma_mapping_ops;
}

static void vio_cmo_bus_init(void)
{
	struct hvcall_mpp_data mpp_data;
	int err;

	memset(&vio_cmo, 0, sizeof(struct vio_cmo));
	spin_lock_init(&vio_cmo.lock);
	INIT_LIST_HEAD(&vio_cmo.device_list);
	INIT_DELAYED_WORK(&vio_cmo.balance_q, vio_cmo_balance);

	/* Get current system entitlement */
	err = h_get_mpp(&mpp_data);

	/*
	 * On failure, continue with entitlement set to 0, will panic()
	 * later when spare is reserved.
	 */
	if (err != H_SUCCESS) {
		printk(KERN_ERR "%s: unable to determine system IO "\
		       "entitlement. (%d)\n", __func__, err);
		vio_cmo.entitled = 0;
	} else {
		vio_cmo.entitled = mpp_data.entitled_mem;
	}

	/* Set reservation and check against entitlement */
	vio_cmo.spare = VIO_CMO_MIN_ENT;
	vio_cmo.reserve.size = vio_cmo.spare;
	vio_cmo.reserve.size += (vio_cmo_num_OF_devs() *
	                         VIO_CMO_MIN_ENT);
	if (vio_cmo.reserve.size > vio_cmo.entitled) {
		printk(KERN_ERR "%s: insufficient system entitlement\n",
		       __func__);
		panic("%s: Insufficient system entitlement", __func__);
	}

	/* Set the remaining accounting variables */
	vio_cmo.excess.size = vio_cmo.entitled - vio_cmo.reserve.size;
	vio_cmo.excess.free = vio_cmo.excess.size;
	vio_cmo.min = vio_cmo.reserve.size;
	vio_cmo.desired = vio_cmo.reserve.size;
}

/* sysfs device functions and data structures for CMO */

#define viodev_cmo_rd_attr(name)                                        \
static ssize_t viodev_cmo_##name##_show(struct device *dev,             \
                                        struct device_attribute *attr,  \
                                         char *buf)                     \
{                                                                       \
	return sprintf(buf, "%lu\n", to_vio_dev(dev)->cmo.name);        \
}

static ssize_t viodev_cmo_allocs_failed_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct vio_dev *viodev = to_vio_dev(dev);
	return sprintf(buf, "%d\n", atomic_read(&viodev->cmo.allocs_failed));
}

static ssize_t viodev_cmo_allocs_failed_reset(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct vio_dev *viodev = to_vio_dev(dev);
	atomic_set(&viodev->cmo.allocs_failed, 0);
	return count;
}

static ssize_t viodev_cmo_desired_set(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct vio_dev *viodev = to_vio_dev(dev);
	size_t new_desired;
	int ret;

	ret = strict_strtoul(buf, 10, &new_desired);
	if (ret)
		return ret;

	vio_cmo_set_dev_desired(viodev, new_desired);
	return count;
}

viodev_cmo_rd_attr(desired);
viodev_cmo_rd_attr(entitled);
viodev_cmo_rd_attr(allocated);

static ssize_t name_show(struct device *, struct device_attribute *, char *);
static ssize_t devspec_show(struct device *, struct device_attribute *, char *);
static ssize_t modalias_show(struct device *dev, struct device_attribute *attr,
			     char *buf);
static struct device_attribute vio_cmo_dev_attrs[] = {
	__ATTR_RO(name),
	__ATTR_RO(devspec),
	__ATTR_RO(modalias),
	__ATTR(cmo_desired,       S_IWUSR|S_IRUSR|S_IWGRP|S_IRGRP|S_IROTH,
	       viodev_cmo_desired_show, viodev_cmo_desired_set),
	__ATTR(cmo_entitled,      S_IRUGO, viodev_cmo_entitled_show,      NULL),
	__ATTR(cmo_allocated,     S_IRUGO, viodev_cmo_allocated_show,     NULL),
	__ATTR(cmo_allocs_failed, S_IWUSR|S_IRUSR|S_IWGRP|S_IRGRP|S_IROTH,
	       viodev_cmo_allocs_failed_show, viodev_cmo_allocs_failed_reset),
	__ATTR_NULL
};

/* sysfs bus functions and data structures for CMO */

#define viobus_cmo_rd_attr(name)                                        \
static ssize_t                                                          \
viobus_cmo_##name##_show(struct bus_type *bt, char *buf)                \
{                                                                       \
	return sprintf(buf, "%lu\n", vio_cmo.name);                     \
}

#define viobus_cmo_pool_rd_attr(name, var)                              \
static ssize_t                                                          \
viobus_cmo_##name##_pool_show_##var(struct bus_type *bt, char *buf)     \
{                                                                       \
	return sprintf(buf, "%lu\n", vio_cmo.name.var);                 \
}

static ssize_t viobus_cmo_high_reset(struct bus_type *bt, const char *buf,
                                     size_t count)
{
	unsigned long flags;

	spin_lock_irqsave(&vio_cmo.lock, flags);
	vio_cmo.high = vio_cmo.curr;
	spin_unlock_irqrestore(&vio_cmo.lock, flags);

	return count;
}

viobus_cmo_rd_attr(entitled);
viobus_cmo_pool_rd_attr(reserve, size);
viobus_cmo_pool_rd_attr(excess, size);
viobus_cmo_pool_rd_attr(excess, free);
viobus_cmo_rd_attr(spare);
viobus_cmo_rd_attr(min);
viobus_cmo_rd_attr(desired);
viobus_cmo_rd_attr(curr);
viobus_cmo_rd_attr(high);

static struct bus_attribute vio_cmo_bus_attrs[] = {
	__ATTR(cmo_entitled, S_IRUGO, viobus_cmo_entitled_show, NULL),
	__ATTR(cmo_reserve_size, S_IRUGO, viobus_cmo_reserve_pool_show_size, NULL),
	__ATTR(cmo_excess_size, S_IRUGO, viobus_cmo_excess_pool_show_size, NULL),
	__ATTR(cmo_excess_free, S_IRUGO, viobus_cmo_excess_pool_show_free, NULL),
	__ATTR(cmo_spare,   S_IRUGO, viobus_cmo_spare_show,   NULL),
	__ATTR(cmo_min,     S_IRUGO, viobus_cmo_min_show,     NULL),
	__ATTR(cmo_desired, S_IRUGO, viobus_cmo_desired_show, NULL),
	__ATTR(cmo_curr,    S_IRUGO, viobus_cmo_curr_show,    NULL),
	__ATTR(cmo_high,    S_IWUSR|S_IRUSR|S_IWGRP|S_IRGRP|S_IROTH,
	       viobus_cmo_high_show, viobus_cmo_high_reset),
	__ATTR_NULL
};

static void vio_cmo_sysfs_init(void)
{
	vio_bus_type.dev_attrs = vio_cmo_dev_attrs;
	vio_bus_type.bus_attrs = vio_cmo_bus_attrs;
}
#else /* CONFIG_PPC_SMLPAR */
/* Dummy functions for iSeries platform */
int vio_cmo_entitlement_update(size_t new_entitlement) { return 0; }
void vio_cmo_set_dev_desired(struct vio_dev *viodev, size_t desired) {}
static int vio_cmo_bus_probe(struct vio_dev *viodev) { return 0; }
static void vio_cmo_bus_remove(struct vio_dev *viodev) {}
static void vio_cmo_set_dma_ops(struct vio_dev *viodev) {}
static void vio_cmo_bus_init(void) {}
static void vio_cmo_sysfs_init(void) { }
#endif /* CONFIG_PPC_SMLPAR */
EXPORT_SYMBOL(vio_cmo_entitlement_update);
EXPORT_SYMBOL(vio_cmo_set_dev_desired);

static struct iommu_table *vio_build_iommu_table(struct vio_dev *dev)
{
	const unsigned char *dma_window;
	struct iommu_table *tbl;
	unsigned long offset, size;

	if (firmware_has_feature(FW_FEATURE_ISERIES))
		return vio_build_iommu_table_iseries(dev);

	dma_window = of_get_property(dev->dev.of_node,
				  "ibm,my-dma-window", NULL);
	if (!dma_window)
		return NULL;

	tbl = kmalloc(sizeof(*tbl), GFP_KERNEL);
	if (tbl == NULL)
		return NULL;

	of_parse_dma_window(dev->dev.of_node, dma_window,
			    &tbl->it_index, &offset, &size);

	/* TCE table size - measured in tce entries */
	tbl->it_size = size >> IOMMU_PAGE_SHIFT;
	/* offset for VIO should always be 0 */
	tbl->it_offset = offset >> IOMMU_PAGE_SHIFT;
	tbl->it_busno = 0;
	tbl->it_type = TCE_VB;

	return iommu_init_table(tbl, -1);
}

static const struct vio_device_id *vio_match_device(
		const struct vio_device_id *ids, const struct vio_dev *dev)
{
	while (ids->type[0] != '\0') {
		if ((strncmp(dev->type, ids->type, strlen(ids->type)) == 0) &&
		    of_device_is_compatible(dev->dev.of_node,
					 ids->compat))
			return ids;
		ids++;
	}
	return NULL;
}

static int vio_bus_probe(struct device *dev)
{
	struct vio_dev *viodev = to_vio_dev(dev);
	struct vio_driver *viodrv = to_vio_driver(dev->driver);
	const struct vio_device_id *id;
	int error = -ENODEV;

	if (!viodrv->probe)
		return error;

	id = vio_match_device(viodrv->id_table, viodev);
	if (id) {
		memset(&viodev->cmo, 0, sizeof(viodev->cmo));
		if (firmware_has_feature(FW_FEATURE_CMO)) {
			error = vio_cmo_bus_probe(viodev);
			if (error)
				return error;
		}
		error = viodrv->probe(viodev, id);
		if (error && firmware_has_feature(FW_FEATURE_CMO))
			vio_cmo_bus_remove(viodev);
	}

	return error;
}

/* convert from struct device to struct vio_dev and pass to driver. */
static int vio_bus_remove(struct device *dev)
{
	struct vio_dev *viodev = to_vio_dev(dev);
	struct vio_driver *viodrv = to_vio_driver(dev->driver);
	struct device *devptr;
	int ret = 1;

	/*
	 * Hold a reference to the device after the remove function is called
	 * to allow for CMO accounting cleanup for the device.
	 */
	devptr = get_device(dev);

	if (viodrv->remove)
		ret = viodrv->remove(viodev);

	if (!ret && firmware_has_feature(FW_FEATURE_CMO))
		vio_cmo_bus_remove(viodev);

	put_device(devptr);
	return ret;
}

int vio_register_driver(struct vio_driver *viodrv)
{
	printk(KERN_DEBUG "%s: driver %s registering\n", __func__,
		viodrv->driver.name);

	/* fill in 'struct driver' fields */
	viodrv->driver.bus = &vio_bus_type;

	return driver_register(&viodrv->driver);
}
EXPORT_SYMBOL(vio_register_driver);

void vio_unregister_driver(struct vio_driver *viodrv)
{
	driver_unregister(&viodrv->driver);
}
EXPORT_SYMBOL(vio_unregister_driver);

/* vio_dev refcount hit 0 */
static void __devinit vio_dev_release(struct device *dev)
{
	/* XXX should free TCE table */
	of_node_put(dev->of_node);
	kfree(to_vio_dev(dev));
}

struct vio_dev *vio_register_device_node(struct device_node *of_node)
{
	struct vio_dev *viodev;
	const unsigned int *unit_address;

	/* we need the 'device_type' property, in order to match with drivers */
	if (of_node->type == NULL) {
		printk(KERN_WARNING "%s: node %s missing 'device_type'\n",
				__func__,
				of_node->name ? of_node->name : "<unknown>");
		return NULL;
	}

	unit_address = of_get_property(of_node, "reg", NULL);
	if (unit_address == NULL) {
		printk(KERN_WARNING "%s: node %s missing 'reg'\n",
				__func__,
				of_node->name ? of_node->name : "<unknown>");
		return NULL;
	}

	/* allocate a vio_dev for this node */
	viodev = kzalloc(sizeof(struct vio_dev), GFP_KERNEL);
	if (viodev == NULL)
		return NULL;

	viodev->irq = irq_of_parse_and_map(of_node, 0);

	dev_set_name(&viodev->dev, "%x", *unit_address);
	viodev->name = of_node->name;
	viodev->type = of_node->type;
	viodev->unit_address = *unit_address;
	if (firmware_has_feature(FW_FEATURE_ISERIES)) {
		unit_address = of_get_property(of_node,
				"linux,unit_address", NULL);
		if (unit_address != NULL)
			viodev->unit_address = *unit_address;
	}
	viodev->dev.of_node = of_node_get(of_node);

	if (firmware_has_feature(FW_FEATURE_CMO))
		vio_cmo_set_dma_ops(viodev);
	else
		viodev->dev.archdata.dma_ops = &dma_iommu_ops;
	set_iommu_table_base(&viodev->dev, vio_build_iommu_table(viodev));
	set_dev_node(&viodev->dev, of_node_to_nid(of_node));

	/* init generic 'struct device' fields: */
	viodev->dev.parent = &vio_bus_device.dev;
	viodev->dev.bus = &vio_bus_type;
	viodev->dev.release = vio_dev_release;

	/* register with generic device framework */
	if (device_register(&viodev->dev)) {
		printk(KERN_ERR "%s: failed to register device %s\n",
				__func__, dev_name(&viodev->dev));
		/* XXX free TCE table */
		kfree(viodev);
		return NULL;
	}

	return viodev;
}
EXPORT_SYMBOL(vio_register_device_node);

static int __init vio_bus_init(void)
{
	int err;
	struct device_node *node_vroot;

	if (firmware_has_feature(FW_FEATURE_CMO))
		vio_cmo_sysfs_init();

	err = bus_register(&vio_bus_type);
	if (err) {
		printk(KERN_ERR "failed to register VIO bus\n");
		return err;
	}

	/*
	 * The fake parent of all vio devices, just to give us
	 * a nice directory
	 */
	err = device_register(&vio_bus_device.dev);
	if (err) {
		printk(KERN_WARNING "%s: device_register returned %i\n",
				__func__, err);
		return err;
	}

	if (firmware_has_feature(FW_FEATURE_CMO))
		vio_cmo_bus_init();

	node_vroot = of_find_node_by_name(NULL, "vdevice");
	if (node_vroot) {
		struct device_node *of_node;

		/*
		 * Create struct vio_devices for each virtual device in
		 * the device tree. Drivers will associate with them later.
		 */
		for (of_node = node_vroot->child; of_node != NULL;
				of_node = of_node->sibling)
			vio_register_device_node(of_node);
		of_node_put(node_vroot);
	}

	return 0;
}
__initcall(vio_bus_init);

static ssize_t name_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", to_vio_dev(dev)->name);
}

static ssize_t devspec_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct device_node *of_node = dev->of_node;

	return sprintf(buf, "%s\n", of_node ? of_node->full_name : "none");
}

static ssize_t modalias_show(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	const struct vio_dev *vio_dev = to_vio_dev(dev);
	struct device_node *dn;
	const char *cp;

	dn = dev->of_node;
	if (!dn)
		return -ENODEV;
	cp = of_get_property(dn, "compatible", NULL);
	if (!cp)
		return -ENODEV;

	return sprintf(buf, "vio:T%sS%s\n", vio_dev->type, cp);
}

static struct device_attribute vio_dev_attrs[] = {
	__ATTR_RO(name),
	__ATTR_RO(devspec),
	__ATTR_RO(modalias),
	__ATTR_NULL
};

void __devinit vio_unregister_device(struct vio_dev *viodev)
{
	device_unregister(&viodev->dev);
}
EXPORT_SYMBOL(vio_unregister_device);

static int vio_bus_match(struct device *dev, struct device_driver *drv)
{
	const struct vio_dev *vio_dev = to_vio_dev(dev);
	struct vio_driver *vio_drv = to_vio_driver(drv);
	const struct vio_device_id *ids = vio_drv->id_table;

	return (ids != NULL) && (vio_match_device(ids, vio_dev) != NULL);
}

static int vio_hotplug(struct device *dev, struct kobj_uevent_env *env)
{
	const struct vio_dev *vio_dev = to_vio_dev(dev);
	struct device_node *dn;
	const char *cp;

	dn = dev->of_node;
	if (!dn)
		return -ENODEV;
	cp = of_get_property(dn, "compatible", NULL);
	if (!cp)
		return -ENODEV;

	add_uevent_var(env, "MODALIAS=vio:T%sS%s", vio_dev->type, cp);
	return 0;
}

static struct bus_type vio_bus_type = {
	.name = "vio",
	.dev_attrs = vio_dev_attrs,
	.uevent = vio_hotplug,
	.match = vio_bus_match,
	.probe = vio_bus_probe,
	.remove = vio_bus_remove,
	.pm = GENERIC_SUBSYS_PM_OPS,
};

const void *vio_get_attribute(struct vio_dev *vdev, char *which, int *length)
{
	return of_get_property(vdev->dev.of_node, which, length);
}
EXPORT_SYMBOL(vio_get_attribute);

#ifdef CONFIG_PPC_PSERIES
static struct vio_dev *vio_find_name(const char *name)
{
	struct device *found;

	found = bus_find_device_by_name(&vio_bus_type, NULL, name);
	if (!found)
		return NULL;

	return to_vio_dev(found);
}

struct vio_dev *vio_find_node(struct device_node *vnode)
{
	const uint32_t *unit_address;
	char kobj_name[20];

	/* construct the kobject name from the device node */
	unit_address = of_get_property(vnode, "reg", NULL);
	if (!unit_address)
		return NULL;
	snprintf(kobj_name, sizeof(kobj_name), "%x", *unit_address);

	return vio_find_name(kobj_name);
}
EXPORT_SYMBOL(vio_find_node);

int vio_enable_interrupts(struct vio_dev *dev)
{
	int rc = h_vio_signal(dev->unit_address, VIO_IRQ_ENABLE);
	if (rc != H_SUCCESS)
		printk(KERN_ERR "vio: Error 0x%x enabling interrupts\n", rc);
	return rc;
}
EXPORT_SYMBOL(vio_enable_interrupts);

int vio_disable_interrupts(struct vio_dev *dev)
{
	int rc = h_vio_signal(dev->unit_address, VIO_IRQ_DISABLE);
	if (rc != H_SUCCESS)
		printk(KERN_ERR "vio: Error 0x%x disabling interrupts\n", rc);
	return rc;
}
EXPORT_SYMBOL(vio_disable_interrupts);
#endif /* CONFIG_PPC_PSERIES */
