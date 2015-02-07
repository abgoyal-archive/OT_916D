
#ifndef _DRM_MEM_UTIL_H_
#define _DRM_MEM_UTIL_H_

#include <linux/vmalloc.h>

static __inline__ void *drm_calloc_large(size_t nmemb, size_t size)
{
	if (size != 0 && nmemb > ULONG_MAX / size)
		return NULL;

	if (size * nmemb <= PAGE_SIZE)
	    return kcalloc(nmemb, size, GFP_KERNEL);

	return __vmalloc(size * nmemb,
			 GFP_KERNEL | __GFP_HIGHMEM | __GFP_ZERO, PAGE_KERNEL);
}

/* Modeled after cairo's malloc_ab, it's like calloc but without the zeroing. */
static __inline__ void *drm_malloc_ab(size_t nmemb, size_t size)
{
	if (size != 0 && nmemb > ULONG_MAX / size)
		return NULL;

	if (size * nmemb <= PAGE_SIZE)
	    return kmalloc(nmemb * size, GFP_KERNEL);

	return __vmalloc(size * nmemb,
			 GFP_KERNEL | __GFP_HIGHMEM, PAGE_KERNEL);
}

static __inline void drm_free_large(void *ptr)
{
	if (!is_vmalloc_addr(ptr))
		return kfree(ptr);

	vfree(ptr);
}

#endif
