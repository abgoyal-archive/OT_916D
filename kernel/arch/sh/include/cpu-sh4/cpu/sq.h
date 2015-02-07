
#ifndef __ASM_CPU_SH4_SQ_H
#define __ASM_CPU_SH4_SQ_H

#include <asm/addrspace.h>
#include <asm/page.h>

#define SQ_SIZE                 32
#define SQ_ALIGN_MASK           (~(SQ_SIZE - 1))
#define SQ_ALIGN(addr)          (((addr)+SQ_SIZE-1) & SQ_ALIGN_MASK)

#define SQ_QACR0		(P4SEG_REG_BASE  + 0x38)
#define SQ_QACR1		(P4SEG_REG_BASE  + 0x3c)
#define SQ_ADDRMAX              (P4SEG_STORE_QUE + 0x04000000)

/* arch/sh/kernel/cpu/sh4/sq.c */
unsigned long sq_remap(unsigned long phys, unsigned int size,
		       const char *name, pgprot_t prot);
void sq_unmap(unsigned long vaddr);
void sq_flush_range(unsigned long start, unsigned int len);

#endif /* __ASM_CPU_SH4_SQ_H */
