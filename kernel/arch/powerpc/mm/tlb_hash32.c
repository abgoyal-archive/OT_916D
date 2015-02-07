

#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/highmem.h>
#include <linux/pagemap.h>

#include <asm/tlbflush.h>
#include <asm/tlb.h>

#include "mmu_decl.h"

void flush_hash_entry(struct mm_struct *mm, pte_t *ptep, unsigned long addr)
{
	unsigned long ptephys;

	if (Hash != 0) {
		ptephys = __pa(ptep) & PAGE_MASK;
		flush_hash_pages(mm->context.id, addr, ptephys, 1);
	}
}
EXPORT_SYMBOL(flush_hash_entry);

void flush_tlb_page_nohash(struct vm_area_struct *vma, unsigned long addr)
{
	if (Hash != 0)
		return;
	_tlbie(addr);
}

void tlb_flush(struct mmu_gather *tlb)
{
	if (Hash == 0) {
		/*
		 * 603 needs to flush the whole TLB here since
		 * it doesn't use a hash table.
		 */
		_tlbia();
	}

	/* Push out batch of freed page tables */
	pte_free_finish();
}


#ifdef CONFIG_SMP_750
#define FINISH_FLUSH	smp_send_tlb_invalidate(0)
#else
#define FINISH_FLUSH	do { } while (0)
#endif

static void flush_range(struct mm_struct *mm, unsigned long start,
			unsigned long end)
{
	pmd_t *pmd;
	unsigned long pmd_end;
	int count;
	unsigned int ctx = mm->context.id;

	if (Hash == 0) {
		_tlbia();
		return;
	}
	start &= PAGE_MASK;
	if (start >= end)
		return;
	end = (end - 1) | ~PAGE_MASK;
	pmd = pmd_offset(pud_offset(pgd_offset(mm, start), start), start);
	for (;;) {
		pmd_end = ((start + PGDIR_SIZE) & PGDIR_MASK) - 1;
		if (pmd_end > end)
			pmd_end = end;
		if (!pmd_none(*pmd)) {
			count = ((pmd_end - start) >> PAGE_SHIFT) + 1;
			flush_hash_pages(ctx, start, pmd_val(*pmd), count);
		}
		if (pmd_end == end)
			break;
		start = pmd_end + 1;
		++pmd;
	}
}

void flush_tlb_kernel_range(unsigned long start, unsigned long end)
{
	flush_range(&init_mm, start, end);
	FINISH_FLUSH;
}
EXPORT_SYMBOL(flush_tlb_kernel_range);

void flush_tlb_mm(struct mm_struct *mm)
{
	struct vm_area_struct *mp;

	if (Hash == 0) {
		_tlbia();
		return;
	}

	/*
	 * It is safe to go down the mm's list of vmas when called
	 * from dup_mmap, holding mmap_sem.  It would also be safe from
	 * unmap_region or exit_mmap, but not from vmtruncate on SMP -
	 * but it seems dup_mmap is the only SMP case which gets here.
	 */
	for (mp = mm->mmap; mp != NULL; mp = mp->vm_next)
		flush_range(mp->vm_mm, mp->vm_start, mp->vm_end);
	FINISH_FLUSH;
}
EXPORT_SYMBOL(flush_tlb_mm);

void flush_tlb_page(struct vm_area_struct *vma, unsigned long vmaddr)
{
	struct mm_struct *mm;
	pmd_t *pmd;

	if (Hash == 0) {
		_tlbie(vmaddr);
		return;
	}
	mm = (vmaddr < TASK_SIZE)? vma->vm_mm: &init_mm;
	pmd = pmd_offset(pud_offset(pgd_offset(mm, vmaddr), vmaddr), vmaddr);
	if (!pmd_none(*pmd))
		flush_hash_pages(mm->context.id, vmaddr, pmd_val(*pmd), 1);
	FINISH_FLUSH;
}
EXPORT_SYMBOL(flush_tlb_page);

void flush_tlb_range(struct vm_area_struct *vma, unsigned long start,
		     unsigned long end)
{
	flush_range(vma->vm_mm, start, end);
	FINISH_FLUSH;
}
EXPORT_SYMBOL(flush_tlb_range);
