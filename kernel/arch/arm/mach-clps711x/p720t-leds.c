
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>

#include <mach/hardware.h>
#include <asm/leds.h>
#include <asm/system.h>
#include <asm/mach-types.h>

#include <asm/hardware/clps7111.h>
#include <asm/hardware/ep7212.h>

static void p720t_leds_event(led_event_t ledevt)
{
	unsigned long flags;
	u32 pddr;

	local_irq_save(flags);
	switch(ledevt) {
	case led_idle_start:
		break;

	case led_idle_end:
		break;

	case led_timer:
		pddr = clps_readb(PDDR);
		clps_writeb(pddr ^ 1, PDDR);
		break;

	default:
		break;
	}

	local_irq_restore(flags);
}

static int __init leds_init(void)
{
	if (machine_is_p720t())
		leds_event = p720t_leds_event;

	return 0;
}

arch_initcall(leds_init);
