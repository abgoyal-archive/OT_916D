


#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include "drmP.h"

#if defined(CONFIG_DEBUG_FS)


static struct drm_info_list drm_debugfs_list[] = {
	{"name", drm_name_info, 0},
	{"vm", drm_vm_info, 0},
	{"clients", drm_clients_info, 0},
	{"queues", drm_queues_info, 0},
	{"bufs", drm_bufs_info, 0},
	{"gem_names", drm_gem_name_info, DRIVER_GEM},
	{"gem_objects", drm_gem_object_info, DRIVER_GEM},
#if DRM_DEBUG_CODE
	{"vma", drm_vma_info, 0},
#endif
};
#define DRM_DEBUGFS_ENTRIES ARRAY_SIZE(drm_debugfs_list)


static int drm_debugfs_open(struct inode *inode, struct file *file)
{
	struct drm_info_node *node = inode->i_private;

	return single_open(file, node->info_ent->show, node);
}


static const struct file_operations drm_debugfs_fops = {
	.owner = THIS_MODULE,
	.open = drm_debugfs_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};


int drm_debugfs_create_files(struct drm_info_list *files, int count,
			     struct dentry *root, struct drm_minor *minor)
{
	struct drm_device *dev = minor->dev;
	struct dentry *ent;
	struct drm_info_node *tmp;
	char name[64];
	int i, ret;

	for (i = 0; i < count; i++) {
		u32 features = files[i].driver_features;

		if (features != 0 &&
		    (dev->driver->driver_features & features) != features)
			continue;

		tmp = kmalloc(sizeof(struct drm_info_node), GFP_KERNEL);
		if (tmp == NULL) {
			ret = -1;
			goto fail;
		}
		ent = debugfs_create_file(files[i].name, S_IFREG | S_IRUGO,
					  root, tmp, &drm_debugfs_fops);
		if (!ent) {
			DRM_ERROR("Cannot create /sys/kernel/debug/dri/%s/%s\n",
				  name, files[i].name);
			kfree(tmp);
			ret = -1;
			goto fail;
		}

		tmp->minor = minor;
		tmp->dent = ent;
		tmp->info_ent = &files[i];
		list_add(&(tmp->list), &(minor->debugfs_nodes.list));
	}
	return 0;

fail:
	drm_debugfs_remove_files(files, count, minor);
	return ret;
}
EXPORT_SYMBOL(drm_debugfs_create_files);

int drm_debugfs_init(struct drm_minor *minor, int minor_id,
		     struct dentry *root)
{
	struct drm_device *dev = minor->dev;
	char name[64];
	int ret;

	INIT_LIST_HEAD(&minor->debugfs_nodes.list);
	sprintf(name, "%d", minor_id);
	minor->debugfs_root = debugfs_create_dir(name, root);
	if (!minor->debugfs_root) {
		DRM_ERROR("Cannot create /sys/kernel/debug/dri/%s\n", name);
		return -1;
	}

	ret = drm_debugfs_create_files(drm_debugfs_list, DRM_DEBUGFS_ENTRIES,
				       minor->debugfs_root, minor);
	if (ret) {
		debugfs_remove(minor->debugfs_root);
		minor->debugfs_root = NULL;
		DRM_ERROR("Failed to create core drm debugfs files\n");
		return ret;
	}

	if (dev->driver->debugfs_init) {
		ret = dev->driver->debugfs_init(minor);
		if (ret) {
			DRM_ERROR("DRM: Driver failed to initialize "
				  "/sys/kernel/debug/dri.\n");
			return ret;
		}
	}
	return 0;
}


int drm_debugfs_remove_files(struct drm_info_list *files, int count,
			     struct drm_minor *minor)
{
	struct list_head *pos, *q;
	struct drm_info_node *tmp;
	int i;

	for (i = 0; i < count; i++) {
		list_for_each_safe(pos, q, &minor->debugfs_nodes.list) {
			tmp = list_entry(pos, struct drm_info_node, list);
			if (tmp->info_ent == &files[i]) {
				debugfs_remove(tmp->dent);
				list_del(pos);
				kfree(tmp);
			}
		}
	}
	return 0;
}
EXPORT_SYMBOL(drm_debugfs_remove_files);

int drm_debugfs_cleanup(struct drm_minor *minor)
{
	struct drm_device *dev = minor->dev;

	if (!minor->debugfs_root)
		return 0;

	if (dev->driver->debugfs_cleanup)
		dev->driver->debugfs_cleanup(minor);

	drm_debugfs_remove_files(drm_debugfs_list, DRM_DEBUGFS_ENTRIES, minor);

	debugfs_remove(minor->debugfs_root);
	minor->debugfs_root = NULL;

	return 0;
}

#endif /* CONFIG_DEBUG_FS */

