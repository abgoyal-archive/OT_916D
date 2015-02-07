

#include <linux/kernel.h>
#include <asm/lv1call.h>
#include <asm/ps3.h>


static struct ps3_sys_manager_ops ps3_sys_manager_ops;


void ps3_sys_manager_register_ops(const struct ps3_sys_manager_ops *ops)
{
	BUG_ON(!ops);
	BUG_ON(!ops->dev);
	ps3_sys_manager_ops = ops ? *ops : ps3_sys_manager_ops;
}
EXPORT_SYMBOL_GPL(ps3_sys_manager_register_ops);

void ps3_sys_manager_power_off(void)
{
	if (ps3_sys_manager_ops.power_off)
		ps3_sys_manager_ops.power_off(ps3_sys_manager_ops.dev);

	ps3_sys_manager_halt();
}

void ps3_sys_manager_restart(void)
{
	if (ps3_sys_manager_ops.restart)
		ps3_sys_manager_ops.restart(ps3_sys_manager_ops.dev);

	ps3_sys_manager_halt();
}

void ps3_sys_manager_halt(void)
{
	pr_emerg("System Halted, OK to turn off power\n");
	local_irq_disable();
	while (1)
		lv1_pause(1);
}

