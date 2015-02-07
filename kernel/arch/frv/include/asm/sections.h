

#ifndef _ASM_SECTIONS_H
#define _ASM_SECTIONS_H

#ifndef __ASSEMBLY__

#include <linux/types.h>
#include <asm-generic/sections.h>

#ifdef __KERNEL__

#define __nongpreldata	__attribute__((section(".data")))
#define __nongprelbss	__attribute__((section(".bss")))

extern const void __kernel_image_start, __kernel_image_end, __page_offset;

extern unsigned long __nongprelbss memory_start;
extern unsigned long __nongprelbss memory_end;
extern unsigned long __nongprelbss rom_length;

/* determine if we're running from ROM */
static inline int is_in_rom(unsigned long addr)
{
	return 0; /* default case: not in ROM */
}

#endif
#endif
#endif /* _ASM_SECTIONS_H */
