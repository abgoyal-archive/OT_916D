

#ifndef _CORE_PRIV_H
#define _CORE_PRIV_H

#include <linux/list.h>
#include <linux/spinlock.h>

#include <rdma/ib_verbs.h>

int  ib_device_register_sysfs(struct ib_device *device,
			      int (*port_callback)(struct ib_device *,
						   u8, struct kobject *));
void ib_device_unregister_sysfs(struct ib_device *device);

int  ib_sysfs_setup(void);
void ib_sysfs_cleanup(void);

int  ib_cache_setup(void);
void ib_cache_cleanup(void);

#endif /* _CORE_PRIV_H */
