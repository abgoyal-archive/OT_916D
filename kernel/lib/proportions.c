

#include <linux/proportions.h>
#include <linux/rcupdate.h>

int prop_descriptor_init(struct prop_descriptor *pd, int shift)
{
	int err;

	if (shift > PROP_MAX_SHIFT)
		shift = PROP_MAX_SHIFT;

	pd->index = 0;
	pd->pg[0].shift = shift;
	mutex_init(&pd->mutex);
	err = percpu_counter_init(&pd->pg[0].events, 0);
	if (err)
		goto out;

	err = percpu_counter_init(&pd->pg[1].events, 0);
	if (err)
		percpu_counter_destroy(&pd->pg[0].events);

out:
	return err;
}

void prop_change_shift(struct prop_descriptor *pd, int shift)
{
	int index;
	int offset;
	u64 events;
	unsigned long flags;

	if (shift > PROP_MAX_SHIFT)
		shift = PROP_MAX_SHIFT;

	mutex_lock(&pd->mutex);

	index = pd->index ^ 1;
	offset = pd->pg[pd->index].shift - shift;
	if (!offset)
		goto out;

	pd->pg[index].shift = shift;

	local_irq_save(flags);
	events = percpu_counter_sum(&pd->pg[pd->index].events);
	if (offset < 0)
		events <<= -offset;
	else
		events >>= offset;
	percpu_counter_set(&pd->pg[index].events, events);

	/*
	 * ensure the new pg is fully written before the switch
	 */
	smp_wmb();
	pd->index = index;
	local_irq_restore(flags);

	synchronize_rcu();

out:
	mutex_unlock(&pd->mutex);
}

static struct prop_global *prop_get_global(struct prop_descriptor *pd)
__acquires(RCU)
{
	int index;

	rcu_read_lock();
	index = pd->index;
	/*
	 * match the wmb from vcd_flip()
	 */
	smp_rmb();
	return &pd->pg[index];
}

static void prop_put_global(struct prop_descriptor *pd, struct prop_global *pg)
__releases(RCU)
{
	rcu_read_unlock();
}

static void
prop_adjust_shift(int *pl_shift, unsigned long *pl_period, int new_shift)
{
	int offset = *pl_shift - new_shift;

	if (!offset)
		return;

	if (offset < 0)
		*pl_period <<= -offset;
	else
		*pl_period >>= offset;

	*pl_shift = new_shift;
}


#define PROP_BATCH (8*(1+ilog2(nr_cpu_ids)))

int prop_local_init_percpu(struct prop_local_percpu *pl)
{
	spin_lock_init(&pl->lock);
	pl->shift = 0;
	pl->period = 0;
	return percpu_counter_init(&pl->events, 0);
}

void prop_local_destroy_percpu(struct prop_local_percpu *pl)
{
	percpu_counter_destroy(&pl->events);
}

static
void prop_norm_percpu(struct prop_global *pg, struct prop_local_percpu *pl)
{
	unsigned long period = 1UL << (pg->shift - 1);
	unsigned long period_mask = ~(period - 1);
	unsigned long global_period;
	unsigned long flags;

	global_period = percpu_counter_read(&pg->events);
	global_period &= period_mask;

	/*
	 * Fast path - check if the local and global period count still match
	 * outside of the lock.
	 */
	if (pl->period == global_period)
		return;

	spin_lock_irqsave(&pl->lock, flags);
	prop_adjust_shift(&pl->shift, &pl->period, pg->shift);

	/*
	 * For each missed period, we half the local counter.
	 * basically:
	 *   pl->events >> (global_period - pl->period);
	 */
	period = (global_period - pl->period) >> (pg->shift - 1);
	if (period < BITS_PER_LONG) {
		s64 val = percpu_counter_read(&pl->events);

		if (val < (nr_cpu_ids * PROP_BATCH))
			val = percpu_counter_sum(&pl->events);

		__percpu_counter_add(&pl->events, -val + (val >> period),
					PROP_BATCH);
	} else
		percpu_counter_set(&pl->events, 0);

	pl->period = global_period;
	spin_unlock_irqrestore(&pl->lock, flags);
}

void __prop_inc_percpu(struct prop_descriptor *pd, struct prop_local_percpu *pl)
{
	struct prop_global *pg = prop_get_global(pd);

	prop_norm_percpu(pg, pl);
	__percpu_counter_add(&pl->events, 1, PROP_BATCH);
	percpu_counter_add(&pg->events, 1);
	prop_put_global(pd, pg);
}

void __prop_inc_percpu_max(struct prop_descriptor *pd,
			   struct prop_local_percpu *pl, long frac)
{
	struct prop_global *pg = prop_get_global(pd);

	prop_norm_percpu(pg, pl);

	if (unlikely(frac != PROP_FRAC_BASE)) {
		unsigned long period_2 = 1UL << (pg->shift - 1);
		unsigned long counter_mask = period_2 - 1;
		unsigned long global_count;
		long numerator, denominator;

		numerator = percpu_counter_read_positive(&pl->events);
		global_count = percpu_counter_read(&pg->events);
		denominator = period_2 + (global_count & counter_mask);

		if (numerator > ((denominator * frac) >> PROP_FRAC_SHIFT))
			goto out_put;
	}

	percpu_counter_add(&pl->events, 1);
	percpu_counter_add(&pg->events, 1);

out_put:
	prop_put_global(pd, pg);
}

void prop_fraction_percpu(struct prop_descriptor *pd,
		struct prop_local_percpu *pl,
		long *numerator, long *denominator)
{
	struct prop_global *pg = prop_get_global(pd);
	unsigned long period_2 = 1UL << (pg->shift - 1);
	unsigned long counter_mask = period_2 - 1;
	unsigned long global_count;

	prop_norm_percpu(pg, pl);
	*numerator = percpu_counter_read_positive(&pl->events);

	global_count = percpu_counter_read(&pg->events);
	*denominator = period_2 + (global_count & counter_mask);

	prop_put_global(pd, pg);
}


int prop_local_init_single(struct prop_local_single *pl)
{
	spin_lock_init(&pl->lock);
	pl->shift = 0;
	pl->period = 0;
	pl->events = 0;
	return 0;
}

void prop_local_destroy_single(struct prop_local_single *pl)
{
}

static
void prop_norm_single(struct prop_global *pg, struct prop_local_single *pl)
{
	unsigned long period = 1UL << (pg->shift - 1);
	unsigned long period_mask = ~(period - 1);
	unsigned long global_period;
	unsigned long flags;

	global_period = percpu_counter_read(&pg->events);
	global_period &= period_mask;

	/*
	 * Fast path - check if the local and global period count still match
	 * outside of the lock.
	 */
	if (pl->period == global_period)
		return;

	spin_lock_irqsave(&pl->lock, flags);
	prop_adjust_shift(&pl->shift, &pl->period, pg->shift);
	/*
	 * For each missed period, we half the local counter.
	 */
	period = (global_period - pl->period) >> (pg->shift - 1);
	if (likely(period < BITS_PER_LONG))
		pl->events >>= period;
	else
		pl->events = 0;
	pl->period = global_period;
	spin_unlock_irqrestore(&pl->lock, flags);
}

void __prop_inc_single(struct prop_descriptor *pd, struct prop_local_single *pl)
{
	struct prop_global *pg = prop_get_global(pd);

	prop_norm_single(pg, pl);
	pl->events++;
	percpu_counter_add(&pg->events, 1);
	prop_put_global(pd, pg);
}

void prop_fraction_single(struct prop_descriptor *pd,
	       	struct prop_local_single *pl,
		long *numerator, long *denominator)
{
	struct prop_global *pg = prop_get_global(pd);
	unsigned long period_2 = 1UL << (pg->shift - 1);
	unsigned long counter_mask = period_2 - 1;
	unsigned long global_count;

	prop_norm_single(pg, pl);
	*numerator = pl->events;

	global_count = percpu_counter_read(&pg->events);
	*denominator = period_2 + (global_count & counter_mask);

	prop_put_global(pd, pg);
}
