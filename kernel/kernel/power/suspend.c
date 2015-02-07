

#include <linux/string.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/cpu.h>
#include <linux/syscalls.h>
#include <linux/gfp.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/suspend.h>

#include "power.h"

const char *const pm_states[PM_SUSPEND_MAX] = {
#ifdef CONFIG_EARLYSUSPEND
	[PM_SUSPEND_ON]		= "on",
#endif
	[PM_SUSPEND_STANDBY]	= "standby",
	[PM_SUSPEND_MEM]	= "mem",
};

static struct platform_suspend_ops *suspend_ops;

void suspend_set_ops(struct platform_suspend_ops *ops)
{
	mutex_lock(&pm_mutex);
	suspend_ops = ops;
	mutex_unlock(&pm_mutex);
}

bool valid_state(suspend_state_t state)
{
	/*
	 * All states need lowlevel support and need to be valid to the lowlevel
	 * implementation, no valid callback implies that none are valid.
	 */
	return suspend_ops && suspend_ops->valid && suspend_ops->valid(state);
}

int suspend_valid_only_mem(suspend_state_t state)
{
	return state == PM_SUSPEND_MEM;
}

static int suspend_test(int level)
{
#ifdef CONFIG_PM_DEBUG
	if (pm_test_level == level) {
		printk(KERN_INFO "suspend debug: Waiting for 5 seconds.\n");
		mdelay(5000);
		return 1;
	}
#endif /* !CONFIG_PM_DEBUG */
	return 0;
}

static int suspend_prepare(void)
{
	int error;

	if (!suspend_ops || !suspend_ops->enter)
		return -EPERM;
        printk("[%s]: begin pm_prepare_console\r\n",__FUNCTION__);
printk("%%%%%% enter pm_prepare_console 1\n\r");
#if defined(CONFIG_VT_CONSOLE)
	pm_prepare_console();
#endif
        printk("[%s]: begin pm_notifier_call_chain\r\n",__FUNCTION__);
	error = pm_notifier_call_chain(PM_SUSPEND_PREPARE);
	if (error)
		goto Finish;

        printk("[%s]: begin usermodehelper_disable\r\n",__FUNCTION__);
	error = usermodehelper_disable();
	if (error)
		goto Finish;

        printk("[%s]: begin suspend_freeze_processes\r\n",__FUNCTION__);
	error = suspend_freeze_processes();
	if (!error)
		return 0;

	suspend_thaw_processes();
	usermodehelper_enable();
 Finish:
	pm_notifier_call_chain(PM_POST_SUSPEND);
#if defined(CONFIG_VT_CONSOLE)
	pm_restore_console();
#endif
	return error;
}

/* default implementation */
void __attribute__ ((weak)) arch_suspend_disable_irqs(void)
{
	local_irq_disable();
}

/* default implementation */
void __attribute__ ((weak)) arch_suspend_enable_irqs(void)
{
	local_irq_enable();
}

static int suspend_enter(suspend_state_t state)
{
	int error;

	if (suspend_ops->prepare) {
		error = suspend_ops->prepare();
		if (error)
			return error;
	}

	error = dpm_suspend_noirq(PMSG_SUSPEND);
	if (error) {
		printk(KERN_ERR "PM: Some devices failed to power down\n");
		goto Platfrom_finish;
	}

	if (suspend_ops->prepare_late) {
		error = suspend_ops->prepare_late();
		if (error)
			goto Power_up_devices;
	}

	if (suspend_test(TEST_PLATFORM))
		goto Platform_wake;

	error = disable_nonboot_cpus();
	if (error || suspend_test(TEST_CPUS))
		goto Enable_cpus;

	arch_suspend_disable_irqs();
	BUG_ON(!irqs_disabled());

	error = sysdev_suspend(PMSG_SUSPEND);
	if (!error) {
		if (!suspend_test(TEST_CORE))
			error = suspend_ops->enter(state);
		sysdev_resume();
	}

	arch_suspend_enable_irqs();
	BUG_ON(irqs_disabled());

 Enable_cpus:
	enable_nonboot_cpus();

 Platform_wake:
	if (suspend_ops->wake)
		suspend_ops->wake();

 Power_up_devices:
	dpm_resume_noirq(PMSG_RESUME);

 Platfrom_finish:
	if (suspend_ops->finish)
		suspend_ops->finish();

	return error;
}

int suspend_devices_and_enter(suspend_state_t state)
{
	int error;
	gfp_t saved_mask;

	if (!suspend_ops)
		return -ENOSYS;

	if (suspend_ops->begin) {
		error = suspend_ops->begin(state);
		if (error)
			goto Close;
	}
	suspend_console();
	saved_mask = clear_gfp_allowed_mask(GFP_IOFS);
	suspend_test_start();
	error = dpm_suspend_start(PMSG_SUSPEND);
	if (error) {
		printk(KERN_ERR "PM: Some devices failed to suspend\n");
		goto Recover_platform;
	}
	suspend_test_finish("suspend devices");
	if (suspend_test(TEST_DEVICES))
		goto Recover_platform;

	suspend_enter(state);

 Resume_devices:
	suspend_test_start();
	dpm_resume_end(PMSG_RESUME);
	suspend_test_finish("resume devices");
	set_gfp_allowed_mask(saved_mask);
	resume_console();
 Close:
	if (suspend_ops->end)
		suspend_ops->end();
	return error;

 Recover_platform:
	if (suspend_ops->recover)
		suspend_ops->recover();
	goto Resume_devices;
}

static void suspend_finish(void)
{
	suspend_thaw_processes();
	usermodehelper_enable();
	pm_notifier_call_chain(PM_POST_SUSPEND);
#if defined(CONFIG_VT_CONSOLE)
	pm_restore_console();
#endif
}

int enter_state(suspend_state_t state)
{
	int error;

	if (!valid_state(state))
		return -ENODEV;

	if (!mutex_trylock(&pm_mutex))
		return -EBUSY;

	printk(KERN_INFO "PM: Syncing filesystems ... ");
	sys_sync();
	printk("done.\n");

	pr_debug("PM: Preparing system for %s sleep\n", pm_states[state]);
	error = suspend_prepare();
	if (error)
		goto Unlock;

	if (suspend_test(TEST_FREEZER))
		goto Finish;

	pr_debug("PM: Entering %s sleep\n", pm_states[state]);
	error = suspend_devices_and_enter(state);

 Finish:
	pr_debug("PM: Finishing wakeup.\n");
	suspend_finish();
 Unlock:
	mutex_unlock(&pm_mutex);
	return error;
}

int pm_suspend(suspend_state_t state)
{
	if (state > PM_SUSPEND_ON && state <= PM_SUSPEND_MAX)
		return enter_state(state);
	return -EINVAL;
}
EXPORT_SYMBOL(pm_suspend);
