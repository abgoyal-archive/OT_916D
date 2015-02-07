
#ifndef _ASM_MUTEX_H
#define _ASM_MUTEX_H

#if __LINUX_ARM_ARCH__ < 6
/* On pre-ARMv6 hardware the swp based implementation is the most efficient. */
# include <asm-generic/mutex-xchg.h>
#else

static inline void
__mutex_fastpath_lock(atomic_t *count, void (*fail_fn)(atomic_t *))
{
	int __ex_flag, __res;

	__asm__ (

		"ldrex	%0, [%2]	\n\t"
		"sub	%0, %0, #1	\n\t"
		"strex	%1, %0, [%2]	"

		: "=&r" (__res), "=&r" (__ex_flag)
		: "r" (&(count)->counter)
		: "cc","memory" );

	__res |= __ex_flag;
	if (unlikely(__res != 0))
		fail_fn(count);
}

static inline int
__mutex_fastpath_lock_retval(atomic_t *count, int (*fail_fn)(atomic_t *))
{
	int __ex_flag, __res;

	__asm__ (

		"ldrex	%0, [%2]	\n\t"
		"sub	%0, %0, #1	\n\t"
		"strex	%1, %0, [%2]	"

		: "=&r" (__res), "=&r" (__ex_flag)
		: "r" (&(count)->counter)
		: "cc","memory" );

	__res |= __ex_flag;
	if (unlikely(__res != 0))
		__res = fail_fn(count);
	return __res;
}

static inline void
__mutex_fastpath_unlock(atomic_t *count, void (*fail_fn)(atomic_t *))
{
	int __ex_flag, __res, __orig;

	__asm__ (

		"ldrex	%0, [%3]	\n\t"
		"add	%1, %0, #1	\n\t"
		"strex	%2, %1, [%3]	"

		: "=&r" (__orig), "=&r" (__res), "=&r" (__ex_flag)
		: "r" (&(count)->counter)
		: "cc","memory" );

	__orig |= __ex_flag;
	if (unlikely(__orig != 0))
		fail_fn(count);
}

#define __mutex_slowpath_needs_to_unlock()	1

static inline int
__mutex_fastpath_trylock(atomic_t *count, int (*fail_fn)(atomic_t *))
{
	int __ex_flag, __res, __orig;

	__asm__ (

		"1: ldrex	%0, [%3]	\n\t"
		"subs		%1, %0, #1	\n\t"
		"strexeq	%2, %1, [%3]	\n\t"
		"movlt		%0, #0		\n\t"
		"cmpeq		%2, #0		\n\t"
		"bgt		1b		"

		: "=&r" (__orig), "=&r" (__res), "=&r" (__ex_flag)
		: "r" (&count->counter)
		: "cc", "memory" );

	return __orig;
}

#endif
#endif
