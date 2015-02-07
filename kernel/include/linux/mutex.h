
#ifndef __LINUX_MUTEX_H
#define __LINUX_MUTEX_H

#include <linux/list.h>
#include <linux/spinlock_types.h>
#include <linux/linkage.h>
#include <linux/lockdep.h>

#include <asm/atomic.h>

struct mutex {
	/* 1: unlocked, 0: locked, negative: locked, possible waiters */
	atomic_t		count;
	spinlock_t		wait_lock;
	struct list_head	wait_list;
#if defined(CONFIG_DEBUG_MUTEXES) || defined(CONFIG_SMP)
	struct thread_info	*owner;
#endif
#ifdef CONFIG_DEBUG_MUTEXES
	const char 		*name;
	void			*magic;
#endif
#ifdef CONFIG_DEBUG_LOCK_ALLOC
	struct lockdep_map	dep_map;
#endif
};

struct mutex_waiter {
	struct list_head	list;
	struct task_struct	*task;
#ifdef CONFIG_DEBUG_MUTEXES
	void			*magic;
#endif
};

#ifdef CONFIG_DEBUG_MUTEXES
# include <linux/mutex-debug.h>
#else
# define __DEBUG_MUTEX_INITIALIZER(lockname)
# define mutex_init(mutex) \
do {							\
	static struct lock_class_key __key;		\
							\
	__mutex_init((mutex), #mutex, &__key);		\
} while (0)
# define mutex_destroy(mutex)				do { } while (0)
#endif

#ifdef CONFIG_DEBUG_LOCK_ALLOC
# define __DEP_MAP_MUTEX_INITIALIZER(lockname) \
		, .dep_map = { .name = #lockname }
#else
# define __DEP_MAP_MUTEX_INITIALIZER(lockname)
#endif

#define __MUTEX_INITIALIZER(lockname) \
		{ .count = ATOMIC_INIT(1) \
		, .wait_lock = __SPIN_LOCK_UNLOCKED(lockname.wait_lock) \
		, .wait_list = LIST_HEAD_INIT(lockname.wait_list) \
		__DEBUG_MUTEX_INITIALIZER(lockname) \
		__DEP_MAP_MUTEX_INITIALIZER(lockname) }

#define DEFINE_MUTEX(mutexname) \
	struct mutex mutexname = __MUTEX_INITIALIZER(mutexname)

extern void __mutex_init(struct mutex *lock, const char *name,
			 struct lock_class_key *key);

static inline int mutex_is_locked(struct mutex *lock)
{
	return atomic_read(&lock->count) != 1;
}

#ifdef CONFIG_DEBUG_LOCK_ALLOC
extern void mutex_lock_nested(struct mutex *lock, unsigned int subclass);
extern int __must_check mutex_lock_interruptible_nested(struct mutex *lock,
					unsigned int subclass);
extern int __must_check mutex_lock_killable_nested(struct mutex *lock,
					unsigned int subclass);

#define mutex_lock(lock) mutex_lock_nested(lock, 0)
#define mutex_lock_interruptible(lock) mutex_lock_interruptible_nested(lock, 0)
#define mutex_lock_killable(lock) mutex_lock_killable_nested(lock, 0)
#else
extern void mutex_lock(struct mutex *lock);
extern int __must_check mutex_lock_interruptible(struct mutex *lock);
extern int __must_check mutex_lock_killable(struct mutex *lock);

# define mutex_lock_nested(lock, subclass) mutex_lock(lock)
# define mutex_lock_interruptible_nested(lock, subclass) mutex_lock_interruptible(lock)
# define mutex_lock_killable_nested(lock, subclass) mutex_lock_killable(lock)
#endif

extern int mutex_trylock(struct mutex *lock);
extern void mutex_unlock(struct mutex *lock);
extern int atomic_dec_and_mutex_lock(atomic_t *cnt, struct mutex *lock);

#endif
