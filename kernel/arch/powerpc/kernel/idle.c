

#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/smp.h>
#include <linux/cpu.h>
#include <linux/sysctl.h>
#include <linux/tick.h>

#include <asm/system.h>
#include <asm/processor.h>
#include <asm/cputable.h>
#include <asm/time.h>
#include <asm/machdep.h>
#include <asm/smp.h>

#ifdef CONFIG_HOTPLUG_CPU
#define cpu_should_die()	cpu_is_offline(smp_processor_id())
#else
#define cpu_should_die()	0
#endif

static int __init powersave_off(char *arg)
{
	ppc_md.power_save = NULL;
	return 0;
}
__setup("powersave=off", powersave_off);

void cpu_idle(void)
{
	if (ppc_md.idle_loop)
		ppc_md.idle_loop();	/* doesn't return */

	set_thread_flag(TIF_POLLING_NRFLAG);
	while (1) {
		tick_nohz_stop_sched_tick(1);
		while (!need_resched() && !cpu_should_die()) {
			ppc64_runlatch_off();

			if (ppc_md.power_save) {
				clear_thread_flag(TIF_POLLING_NRFLAG);
				/*
				 * smp_mb is so clearing of TIF_POLLING_NRFLAG
				 * is ordered w.r.t. need_resched() test.
				 */
				smp_mb();
				local_irq_disable();

				/* Don't trace irqs off for idle */
				stop_critical_timings();

				/* check again after disabling irqs */
				if (!need_resched() && !cpu_should_die())
					ppc_md.power_save();

				start_critical_timings();

				local_irq_enable();
				set_thread_flag(TIF_POLLING_NRFLAG);

			} else {
				/*
				 * Go into low thread priority and possibly
				 * low power mode.
				 */
				HMT_low();
				HMT_very_low();
			}
		}

		HMT_medium();
		ppc64_runlatch_on();
		tick_nohz_restart_sched_tick();
		if (cpu_should_die())
			cpu_die();
		preempt_enable_no_resched();
		schedule();
		preempt_disable();
	}
}

int powersave_nap;

#ifdef CONFIG_SYSCTL
static ctl_table powersave_nap_ctl_table[]={
	{
		.procname	= "powersave-nap",
		.data		= &powersave_nap,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec,
	},
	{}
};
static ctl_table powersave_nap_sysctl_root[] = {
	{
		.procname	= "kernel",
		.mode		= 0555,
		.child		= powersave_nap_ctl_table,
	},
	{}
};

static int __init
register_powersave_nap_sysctl(void)
{
	register_sysctl_table(powersave_nap_sysctl_root);

	return 0;
}
__initcall(register_powersave_nap_sysctl);
#endif
