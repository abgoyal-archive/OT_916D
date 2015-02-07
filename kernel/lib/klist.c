

#include <linux/klist.h>
#include <linux/module.h>
#include <linux/sched.h>

#define KNODE_DEAD		1LU
#define KNODE_KLIST_MASK	~KNODE_DEAD

static struct klist *knode_klist(struct klist_node *knode)
{
	return (struct klist *)
		((unsigned long)knode->n_klist & KNODE_KLIST_MASK);
}

static bool knode_dead(struct klist_node *knode)
{
	return (unsigned long)knode->n_klist & KNODE_DEAD;
}

static void knode_set_klist(struct klist_node *knode, struct klist *klist)
{
	knode->n_klist = klist;
	/* no knode deserves to start its life dead */
	WARN_ON(knode_dead(knode));
}

static void knode_kill(struct klist_node *knode)
{
	/* and no knode should die twice ever either, see we're very humane */
	WARN_ON(knode_dead(knode));
	*(unsigned long *)&knode->n_klist |= KNODE_DEAD;
}

void klist_init(struct klist *k, void (*get)(struct klist_node *),
		void (*put)(struct klist_node *))
{
	INIT_LIST_HEAD(&k->k_list);
	spin_lock_init(&k->k_lock);
	k->get = get;
	k->put = put;
}
EXPORT_SYMBOL_GPL(klist_init);

static void add_head(struct klist *k, struct klist_node *n)
{
	spin_lock(&k->k_lock);
	list_add(&n->n_node, &k->k_list);
	spin_unlock(&k->k_lock);
}

static void add_tail(struct klist *k, struct klist_node *n)
{
	spin_lock(&k->k_lock);
	list_add_tail(&n->n_node, &k->k_list);
	spin_unlock(&k->k_lock);
}

static void klist_node_init(struct klist *k, struct klist_node *n)
{
	INIT_LIST_HEAD(&n->n_node);
	kref_init(&n->n_ref);
	knode_set_klist(n, k);
	if (k->get)
		k->get(n);
}

void klist_add_head(struct klist_node *n, struct klist *k)
{
	klist_node_init(k, n);
	add_head(k, n);
}
EXPORT_SYMBOL_GPL(klist_add_head);

void klist_add_tail(struct klist_node *n, struct klist *k)
{
	klist_node_init(k, n);
	add_tail(k, n);
}
EXPORT_SYMBOL_GPL(klist_add_tail);

void klist_add_after(struct klist_node *n, struct klist_node *pos)
{
	struct klist *k = knode_klist(pos);

	klist_node_init(k, n);
	spin_lock(&k->k_lock);
	list_add(&n->n_node, &pos->n_node);
	spin_unlock(&k->k_lock);
}
EXPORT_SYMBOL_GPL(klist_add_after);

void klist_add_before(struct klist_node *n, struct klist_node *pos)
{
	struct klist *k = knode_klist(pos);

	klist_node_init(k, n);
	spin_lock(&k->k_lock);
	list_add_tail(&n->n_node, &pos->n_node);
	spin_unlock(&k->k_lock);
}
EXPORT_SYMBOL_GPL(klist_add_before);

struct klist_waiter {
	struct list_head list;
	struct klist_node *node;
	struct task_struct *process;
	int woken;
};

static DEFINE_SPINLOCK(klist_remove_lock);
static LIST_HEAD(klist_remove_waiters);

static void klist_release(struct kref *kref)
{
	struct klist_waiter *waiter, *tmp;
	struct klist_node *n = container_of(kref, struct klist_node, n_ref);

	WARN_ON(!knode_dead(n));
	list_del(&n->n_node);
	spin_lock(&klist_remove_lock);
	list_for_each_entry_safe(waiter, tmp, &klist_remove_waiters, list) {
		if (waiter->node != n)
			continue;

		waiter->woken = 1;
		mb();
		wake_up_process(waiter->process);
		list_del(&waiter->list);
	}
	spin_unlock(&klist_remove_lock);
	knode_set_klist(n, NULL);
}

static int klist_dec_and_del(struct klist_node *n)
{
	return kref_put(&n->n_ref, klist_release);
}

static void klist_put(struct klist_node *n, bool kill)
{
	struct klist *k = knode_klist(n);
	void (*put)(struct klist_node *) = k->put;

	spin_lock(&k->k_lock);
	if (kill)
		knode_kill(n);
	if (!klist_dec_and_del(n))
		put = NULL;
	spin_unlock(&k->k_lock);
	if (put)
		put(n);
}

void klist_del(struct klist_node *n)
{
	klist_put(n, true);
}
EXPORT_SYMBOL_GPL(klist_del);

void klist_remove(struct klist_node *n)
{
	struct klist_waiter waiter;

	waiter.node = n;
	waiter.process = current;
	waiter.woken = 0;
	spin_lock(&klist_remove_lock);
	list_add(&waiter.list, &klist_remove_waiters);
	spin_unlock(&klist_remove_lock);

	klist_del(n);

	for (;;) {
		set_current_state(TASK_UNINTERRUPTIBLE);
		if (waiter.woken)
			break;
		schedule();
	}
	__set_current_state(TASK_RUNNING);
}
EXPORT_SYMBOL_GPL(klist_remove);

int klist_node_attached(struct klist_node *n)
{
	return (n->n_klist != NULL);
}
EXPORT_SYMBOL_GPL(klist_node_attached);

void klist_iter_init_node(struct klist *k, struct klist_iter *i,
			  struct klist_node *n)
{
	i->i_klist = k;
	i->i_cur = n;
	if (n)
		kref_get(&n->n_ref);
}
EXPORT_SYMBOL_GPL(klist_iter_init_node);

void klist_iter_init(struct klist *k, struct klist_iter *i)
{
	klist_iter_init_node(k, i, NULL);
}
EXPORT_SYMBOL_GPL(klist_iter_init);

void klist_iter_exit(struct klist_iter *i)
{
	if (i->i_cur) {
		klist_put(i->i_cur, false);
		i->i_cur = NULL;
	}
}
EXPORT_SYMBOL_GPL(klist_iter_exit);

static struct klist_node *to_klist_node(struct list_head *n)
{
	return container_of(n, struct klist_node, n_node);
}

struct klist_node *klist_next(struct klist_iter *i)
{
	void (*put)(struct klist_node *) = i->i_klist->put;
	struct klist_node *last = i->i_cur;
	struct klist_node *next;

	spin_lock(&i->i_klist->k_lock);

	if (last) {
		next = to_klist_node(last->n_node.next);
		if (!klist_dec_and_del(last))
			put = NULL;
	} else
		next = to_klist_node(i->i_klist->k_list.next);

	i->i_cur = NULL;
	while (next != to_klist_node(&i->i_klist->k_list)) {
		if (likely(!knode_dead(next))) {
			kref_get(&next->n_ref);
			i->i_cur = next;
			break;
		}
		next = to_klist_node(next->n_node.next);
	}

	spin_unlock(&i->i_klist->k_lock);

	if (put && last)
		put(last);
	return i->i_cur;
}
EXPORT_SYMBOL_GPL(klist_next);
