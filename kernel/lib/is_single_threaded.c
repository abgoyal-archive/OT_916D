

#include <linux/sched.h>

bool current_is_single_threaded(void)
{
	struct task_struct *task = current;
	struct mm_struct *mm = task->mm;
	struct task_struct *p, *t;
	bool ret;

	if (atomic_read(&task->signal->live) != 1)
		return false;

	if (atomic_read(&mm->mm_users) == 1)
		return true;

	ret = false;
	rcu_read_lock();
	for_each_process(p) {
		if (unlikely(p->flags & PF_KTHREAD))
			continue;
		if (unlikely(p == task->group_leader))
			continue;

		t = p;
		do {
			if (unlikely(t->mm == mm))
				goto found;
			if (likely(t->mm))
				break;
			/*
			 * t->mm == NULL. Make sure next_thread/next_task
			 * will see other CLONE_VM tasks which might be
			 * forked before exiting.
			 */
			smp_rmb();
		} while_each_thread(p, t);
	}
	ret = true;
found:
	rcu_read_unlock();

	return ret;
}
