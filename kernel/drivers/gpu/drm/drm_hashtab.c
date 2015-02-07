

#include "drmP.h"
#include "drm_hashtab.h"
#include <linux/hash.h>
#include <linux/slab.h>

int drm_ht_create(struct drm_open_hash *ht, unsigned int order)
{
	unsigned int i;

	ht->size = 1 << order;
	ht->order = order;
	ht->fill = 0;
	ht->table = NULL;
	ht->use_vmalloc = ((ht->size * sizeof(*ht->table)) > PAGE_SIZE);
	if (!ht->use_vmalloc) {
		ht->table = kcalloc(ht->size, sizeof(*ht->table), GFP_KERNEL);
	}
	if (!ht->table) {
		ht->use_vmalloc = 1;
		ht->table = vmalloc(ht->size*sizeof(*ht->table));
	}
	if (!ht->table) {
		DRM_ERROR("Out of memory for hash table\n");
		return -ENOMEM;
	}
	for (i=0; i< ht->size; ++i) {
		INIT_HLIST_HEAD(&ht->table[i]);
	}
	return 0;
}
EXPORT_SYMBOL(drm_ht_create);

void drm_ht_verbose_list(struct drm_open_hash *ht, unsigned long key)
{
	struct drm_hash_item *entry;
	struct hlist_head *h_list;
	struct hlist_node *list;
	unsigned int hashed_key;
	int count = 0;

	hashed_key = hash_long(key, ht->order);
	DRM_DEBUG("Key is 0x%08lx, Hashed key is 0x%08x\n", key, hashed_key);
	h_list = &ht->table[hashed_key];
	hlist_for_each(list, h_list) {
		entry = hlist_entry(list, struct drm_hash_item, head);
		DRM_DEBUG("count %d, key: 0x%08lx\n", count++, entry->key);
	}
}

static struct hlist_node *drm_ht_find_key(struct drm_open_hash *ht,
					  unsigned long key)
{
	struct drm_hash_item *entry;
	struct hlist_head *h_list;
	struct hlist_node *list;
	unsigned int hashed_key;

	hashed_key = hash_long(key, ht->order);
	h_list = &ht->table[hashed_key];
	hlist_for_each(list, h_list) {
		entry = hlist_entry(list, struct drm_hash_item, head);
		if (entry->key == key)
			return list;
		if (entry->key > key)
			break;
	}
	return NULL;
}


int drm_ht_insert_item(struct drm_open_hash *ht, struct drm_hash_item *item)
{
	struct drm_hash_item *entry;
	struct hlist_head *h_list;
	struct hlist_node *list, *parent;
	unsigned int hashed_key;
	unsigned long key = item->key;

	hashed_key = hash_long(key, ht->order);
	h_list = &ht->table[hashed_key];
	parent = NULL;
	hlist_for_each(list, h_list) {
		entry = hlist_entry(list, struct drm_hash_item, head);
		if (entry->key == key)
			return -EINVAL;
		if (entry->key > key)
			break;
		parent = list;
	}
	if (parent) {
		hlist_add_after(parent, &item->head);
	} else {
		hlist_add_head(&item->head, h_list);
	}
	return 0;
}
EXPORT_SYMBOL(drm_ht_insert_item);

int drm_ht_just_insert_please(struct drm_open_hash *ht, struct drm_hash_item *item,
			      unsigned long seed, int bits, int shift,
			      unsigned long add)
{
	int ret;
	unsigned long mask = (1 << bits) - 1;
	unsigned long first, unshifted_key;

	unshifted_key = hash_long(seed, bits);
	first = unshifted_key;
	do {
		item->key = (unshifted_key << shift) + add;
		ret = drm_ht_insert_item(ht, item);
		if (ret)
			unshifted_key = (unshifted_key + 1) & mask;
	} while(ret && (unshifted_key != first));

	if (ret) {
		DRM_ERROR("Available key bit space exhausted\n");
		return -EINVAL;
	}
	return 0;
}
EXPORT_SYMBOL(drm_ht_just_insert_please);

int drm_ht_find_item(struct drm_open_hash *ht, unsigned long key,
		     struct drm_hash_item **item)
{
	struct hlist_node *list;

	list = drm_ht_find_key(ht, key);
	if (!list)
		return -EINVAL;

	*item = hlist_entry(list, struct drm_hash_item, head);
	return 0;
}
EXPORT_SYMBOL(drm_ht_find_item);

int drm_ht_remove_key(struct drm_open_hash *ht, unsigned long key)
{
	struct hlist_node *list;

	list = drm_ht_find_key(ht, key);
	if (list) {
		hlist_del_init(list);
		ht->fill--;
		return 0;
	}
	return -EINVAL;
}

int drm_ht_remove_item(struct drm_open_hash *ht, struct drm_hash_item *item)
{
	hlist_del_init(&item->head);
	ht->fill--;
	return 0;
}
EXPORT_SYMBOL(drm_ht_remove_item);

void drm_ht_remove(struct drm_open_hash *ht)
{
	if (ht->table) {
		if (ht->use_vmalloc)
			vfree(ht->table);
		else
			kfree(ht->table);
		ht->table = NULL;
	}
}
EXPORT_SYMBOL(drm_ht_remove);
