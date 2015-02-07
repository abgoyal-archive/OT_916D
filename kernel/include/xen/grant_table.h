

#ifndef __ASM_GNTTAB_H__
#define __ASM_GNTTAB_H__

#include <asm/xen/hypervisor.h>
#include <xen/interface/grant_table.h>
#include <asm/xen/grant_table.h>

/* NR_GRANT_FRAMES must be less than or equal to that configured in Xen */
#define NR_GRANT_FRAMES 4

struct gnttab_free_callback {
	struct gnttab_free_callback *next;
	void (*fn)(void *);
	void *arg;
	u16 count;
};

int gnttab_suspend(void);
int gnttab_resume(void);

int gnttab_grant_foreign_access(domid_t domid, unsigned long frame,
				int readonly);

int gnttab_end_foreign_access_ref(grant_ref_t ref, int readonly);

void gnttab_end_foreign_access(grant_ref_t ref, int readonly,
			       unsigned long page);

int gnttab_grant_foreign_transfer(domid_t domid, unsigned long pfn);

unsigned long gnttab_end_foreign_transfer_ref(grant_ref_t ref);
unsigned long gnttab_end_foreign_transfer(grant_ref_t ref);

int gnttab_query_foreign_access(grant_ref_t ref);

int gnttab_alloc_grant_references(u16 count, grant_ref_t *pprivate_head);

void gnttab_free_grant_reference(grant_ref_t ref);

void gnttab_free_grant_references(grant_ref_t head);

int gnttab_empty_grant_references(const grant_ref_t *pprivate_head);

int gnttab_claim_grant_reference(grant_ref_t *pprivate_head);

void gnttab_release_grant_reference(grant_ref_t *private_head,
				    grant_ref_t release);

void gnttab_request_free_callback(struct gnttab_free_callback *callback,
				  void (*fn)(void *), void *arg, u16 count);
void gnttab_cancel_free_callback(struct gnttab_free_callback *callback);

void gnttab_grant_foreign_access_ref(grant_ref_t ref, domid_t domid,
				     unsigned long frame, int readonly);

void gnttab_grant_foreign_transfer_ref(grant_ref_t, domid_t domid,
				       unsigned long pfn);

int arch_gnttab_map_shared(unsigned long *frames, unsigned long nr_gframes,
			   unsigned long max_nr_gframes,
			   struct grant_entry **__shared);
void arch_gnttab_unmap_shared(struct grant_entry *shared,
			      unsigned long nr_gframes);

#define gnttab_map_vaddr(map) ((void *)(map.host_virt_addr))

#endif /* __ASM_GNTTAB_H__ */
