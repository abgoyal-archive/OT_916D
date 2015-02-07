

#ifdef CONFIG_NEED_PER_CPU_PAGE_FIRST_CHUNK
#error "contiguous percpu allocation is incompatible with paged first chunk"
#endif

#include <linux/log2.h>

static int pcpu_populate_chunk(struct pcpu_chunk *chunk, int off, int size)
{
	/* noop */
	return 0;
}

static void pcpu_depopulate_chunk(struct pcpu_chunk *chunk, int off, int size)
{
	/* nada */
}

static struct pcpu_chunk *pcpu_create_chunk(void)
{
	const int nr_pages = pcpu_group_sizes[0] >> PAGE_SHIFT;
	struct pcpu_chunk *chunk;
	struct page *pages;
	int i;

	chunk = pcpu_alloc_chunk();
	if (!chunk)
		return NULL;

	pages = alloc_pages(GFP_KERNEL, order_base_2(nr_pages));
	if (!pages) {
		pcpu_free_chunk(chunk);
		return NULL;
	}

	for (i = 0; i < nr_pages; i++)
		pcpu_set_page_chunk(nth_page(pages, i), chunk);

	chunk->data = pages;
	chunk->base_addr = page_address(pages) - pcpu_group_offsets[0];
	return chunk;
}

static void pcpu_destroy_chunk(struct pcpu_chunk *chunk)
{
	const int nr_pages = pcpu_group_sizes[0] >> PAGE_SHIFT;

	if (chunk && chunk->data)
		__free_pages(chunk->data, order_base_2(nr_pages));
	pcpu_free_chunk(chunk);
}

static struct page *pcpu_addr_to_page(void *addr)
{
	return virt_to_page(addr);
}

static int __init pcpu_verify_alloc_info(const struct pcpu_alloc_info *ai)
{
	size_t nr_pages, alloc_pages;

	/* all units must be in a single group */
	if (ai->nr_groups != 1) {
		printk(KERN_CRIT "percpu: can't handle more than one groups\n");
		return -EINVAL;
	}

	nr_pages = (ai->groups[0].nr_units * ai->unit_size) >> PAGE_SHIFT;
	alloc_pages = roundup_pow_of_two(nr_pages);

	if (alloc_pages > nr_pages)
		printk(KERN_WARNING "percpu: wasting %zu pages per chunk\n",
		       alloc_pages - nr_pages);

	return 0;
}
