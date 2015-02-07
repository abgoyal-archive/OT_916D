
#ifndef __PVRUSB2_SYSFS_H
#define __PVRUSB2_SYSFS_H

#include <linux/list.h>
#include <linux/sysfs.h>
#include "pvrusb2-context.h"

struct pvr2_sysfs;
struct pvr2_sysfs_class;

struct pvr2_sysfs_class *pvr2_sysfs_class_create(void);
void pvr2_sysfs_class_destroy(struct pvr2_sysfs_class *);

struct pvr2_sysfs *pvr2_sysfs_create(struct pvr2_context *,
				     struct pvr2_sysfs_class *);

#endif /* __PVRUSB2_SYSFS_H */

