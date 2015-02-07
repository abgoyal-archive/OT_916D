

#ifndef __LINUX_CNT32_TO_63_H__
#define __LINUX_CNT32_TO_63_H__

#include <linux/compiler.h>
#include <linux/types.h>
#include <asm/byteorder.h>
#include <asm/system.h>

/* this is used only to give gcc a clue about good code generation */
union cnt32_to_63 {
	struct {
#if defined(__LITTLE_ENDIAN)
		u32 lo, hi;
#elif defined(__BIG_ENDIAN)
		u32 hi, lo;
#endif
	};
	u64 val;
};


#define cnt32_to_63(cnt_lo) \
({ \
	static u32 __m_cnt_hi; \
	union cnt32_to_63 __x; \
	__x.hi = __m_cnt_hi; \
 	smp_rmb(); \
	__x.lo = (cnt_lo); \
	if (unlikely((s32)(__x.hi ^ __x.lo) < 0)) \
		__m_cnt_hi = __x.hi = (__x.hi ^ 0x80000000) + (__x.hi >> 31); \
	__x.val; \
})

#endif
