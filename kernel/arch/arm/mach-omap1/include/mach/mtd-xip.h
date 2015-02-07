

#ifndef __ARCH_OMAP_MTD_XIP_H__
#define __ARCH_OMAP_MTD_XIP_H__

#include <mach/hardware.h>
#define OMAP_MPU_TIMER_BASE	(0xfffec500)
#define OMAP_MPU_TIMER_OFFSET	0x100

typedef struct {
	u32 cntl;			/* CNTL_TIMER, R/W */
	u32 load_tim;			/* LOAD_TIM,   W */
	u32 read_tim;			/* READ_TIM,   R */
} xip_omap_mpu_timer_regs_t;

#define xip_omap_mpu_timer_base(n)					\
((volatile xip_omap_mpu_timer_regs_t*)OMAP1_IO_ADDRESS(OMAP_MPU_TIMER_BASE +	\
	(n)*OMAP_MPU_TIMER_OFFSET))

static inline unsigned long xip_omap_mpu_timer_read(int nr)
{
	volatile xip_omap_mpu_timer_regs_t* timer = xip_omap_mpu_timer_base(nr);
	return timer->read_tim;
}

#define xip_irqpending()	\
	(omap_readl(OMAP_IH1_ITR) & ~omap_readl(OMAP_IH1_MIR))
#define xip_currtime()		(~xip_omap_mpu_timer_read(0))


#ifdef CONFIG_MACH_OMAP_PERSEUS2
#define xip_elapsed_since(x)	(signed)((~xip_omap_mpu_timer_read(0) - (x)) / 7)
#else
#define xip_elapsed_since(x)	(signed)((~xip_omap_mpu_timer_read(0) - (x)) / 6)
#endif


#define xip_cpu_idle()  asm volatile ("mcr p15, 0, %0, c7, c0, 4" :: "r" (1))

#endif /* __ARCH_OMAP_MTD_XIP_H__ */
