

#ifndef _XTENSA_SYSTEM_H
#define _XTENSA_SYSTEM_H

#include <linux/stringify.h>

#include <asm/processor.h>

/* interrupt control */

#define local_save_flags(x)						\
	__asm__ __volatile__ ("rsr %0,"__stringify(PS) : "=a" (x));
#define local_irq_restore(x)	do {					\
	__asm__ __volatile__ ("wsr %0, "__stringify(PS)" ; rsync" 	\
	    		      :: "a" (x) : "memory"); } while(0);
#define local_irq_save(x)	do {					\
	__asm__ __volatile__ ("rsil %0, "__stringify(LOCKLEVEL) 	\
	    		      : "=a" (x) :: "memory");} while(0);

static inline void local_irq_disable(void)
{
	unsigned long flags;
	__asm__ __volatile__ ("rsil %0, "__stringify(LOCKLEVEL)
	    		      : "=a" (flags) :: "memory");
}
static inline void local_irq_enable(void)
{
	unsigned long flags;
	__asm__ __volatile__ ("rsil %0, 0" : "=a" (flags) :: "memory");

}

static inline int irqs_disabled(void)
{
	unsigned long flags;
	local_save_flags(flags);
	return flags & 0xf;
}


#define smp_read_barrier_depends() do { } while(0)
#define read_barrier_depends() do { } while(0)

#define mb()  barrier()
#define rmb() mb()
#define wmb() mb()

#ifdef CONFIG_SMP
#error smp_* not defined
#else
#define smp_mb()	barrier()
#define smp_rmb()	barrier()
#define smp_wmb()	barrier()
#endif

#define set_mb(var, value)	do { var = value; mb(); } while (0)

#if !defined (__ASSEMBLY__)

extern void *_switch_to(void *last, void *next);

#endif	/* __ASSEMBLY__ */

#define switch_to(prev,next,last)		\
do {						\
	(last) = _switch_to(prev, next);	\
} while(0)


static inline unsigned long
__cmpxchg_u32(volatile int *p, int old, int new)
{
  __asm__ __volatile__("rsil    a15, "__stringify(LOCKLEVEL)"\n\t"
		       "l32i    %0, %1, 0              \n\t"
		       "bne	%0, %2, 1f             \n\t"
		       "s32i    %3, %1, 0              \n\t"
		       "1:                             \n\t"
		       "wsr     a15, "__stringify(PS)" \n\t"
		       "rsync                          \n\t"
		       : "=&a" (old)
		       : "a" (p), "a" (old), "r" (new)
		       : "a15", "memory");
  return old;
}

extern void __cmpxchg_called_with_bad_pointer(void);

static __inline__ unsigned long
__cmpxchg(volatile void *ptr, unsigned long old, unsigned long new, int size)
{
	switch (size) {
	case 4:  return __cmpxchg_u32(ptr, old, new);
	default: __cmpxchg_called_with_bad_pointer();
		 return old;
	}
}

#define cmpxchg(ptr,o,n)						      \
	({ __typeof__(*(ptr)) _o_ = (o);				      \
	   __typeof__(*(ptr)) _n_ = (n);				      \
	   (__typeof__(*(ptr))) __cmpxchg((ptr), (unsigned long)_o_,	      \
	   			        (unsigned long)_n_, sizeof (*(ptr))); \
	})

#include <asm-generic/cmpxchg-local.h>

static inline unsigned long __cmpxchg_local(volatile void *ptr,
				      unsigned long old,
				      unsigned long new, int size)
{
	switch (size) {
	case 4:
		return __cmpxchg_u32(ptr, old, new);
	default:
		return __cmpxchg_local_generic(ptr, old, new, size);
	}

	return old;
}

#define cmpxchg_local(ptr, o, n)				  	       \
	((__typeof__(*(ptr)))__cmpxchg_local_generic((ptr), (unsigned long)(o),\
			(unsigned long)(n), sizeof(*(ptr))))
#define cmpxchg64_local(ptr, o, n) __cmpxchg64_local_generic((ptr), (o), (n))


static inline unsigned long xchg_u32(volatile int * m, unsigned long val)
{
  unsigned long tmp;
  __asm__ __volatile__("rsil    a15, "__stringify(LOCKLEVEL)"\n\t"
		       "l32i    %0, %1, 0              \n\t"
		       "s32i    %2, %1, 0              \n\t"
		       "wsr     a15, "__stringify(PS)" \n\t"
		       "rsync                          \n\t"
		       : "=&a" (tmp)
		       : "a" (m), "a" (val)
		       : "a15", "memory");
  return tmp;
}

#define xchg(ptr,x) ((__typeof__(*(ptr)))__xchg((unsigned long)(x),(ptr),sizeof(*(ptr))))


extern void __xchg_called_with_bad_pointer(void);

static __inline__ unsigned long
__xchg(unsigned long x, volatile void * ptr, int size)
{
	switch (size) {
		case 4:
			return xchg_u32(ptr, x);
	}
	__xchg_called_with_bad_pointer();
	return x;
}

extern void set_except_vector(int n, void *addr);

static inline void spill_registers(void)
{
	unsigned int a0, ps;

	__asm__ __volatile__ (
		"movi	a14," __stringify (PS_EXCM_BIT) " | 1\n\t"
		"mov	a12, a0\n\t"
		"rsr	a13," __stringify(SAR) "\n\t"
		"xsr	a14," __stringify(PS) "\n\t"
		"movi	a0, _spill_registers\n\t"
		"rsync\n\t"
		"callx0 a0\n\t"
		"mov	a0, a12\n\t"
		"wsr	a13," __stringify(SAR) "\n\t"
		"wsr	a14," __stringify(PS) "\n\t"
		:: "a" (&a0), "a" (&ps)
		: "a2", "a3", "a4", "a7", "a11", "a12", "a13", "a14", "a15", "memory");
}

#define arch_align_stack(x) (x)

#endif	/* _XTENSA_SYSTEM_H */
