
#ifndef __SND_SEQ_MEMORYMGR_H
#define __SND_SEQ_MEMORYMGR_H

#include <sound/seq_kernel.h>
#include <linux/poll.h>

/* container for sequencer event (internal use) */
struct snd_seq_event_cell {
	struct snd_seq_event event;
	struct snd_seq_pool *pool;				/* used pool */
	struct snd_seq_event_cell *next;	/* next cell */
};


struct snd_seq_pool {
	struct snd_seq_event_cell *ptr;	/* pointer to first event chunk */
	struct snd_seq_event_cell *free;	/* pointer to the head of the free list */

	int total_elements;	/* pool size actually allocated */
	atomic_t counter;	/* cells free */

	int size;		/* pool size to be allocated */
	int room;		/* watermark for sleep/wakeup */

	int closing;

	/* statistics */
	int max_used;
	int event_alloc_nopool;
	int event_alloc_failures;
	int event_alloc_success;

	/* Write locking */
	wait_queue_head_t output_sleep;

	/* Pool lock */
	spinlock_t lock;
};

void snd_seq_cell_free(struct snd_seq_event_cell *cell);

int snd_seq_event_dup(struct snd_seq_pool *pool, struct snd_seq_event *event,
		      struct snd_seq_event_cell **cellp, int nonblock, struct file *file);

/* return number of unused (free) cells */
static inline int snd_seq_unused_cells(struct snd_seq_pool *pool)
{
	return pool ? pool->total_elements - atomic_read(&pool->counter) : 0;
}

/* return total number of allocated cells */
static inline int snd_seq_total_cells(struct snd_seq_pool *pool)
{
	return pool ? pool->total_elements : 0;
}

/* init pool - allocate events */
int snd_seq_pool_init(struct snd_seq_pool *pool);

/* done pool - free events */
int snd_seq_pool_done(struct snd_seq_pool *pool);

/* create pool */
struct snd_seq_pool *snd_seq_pool_new(int poolsize);

/* remove pool */
int snd_seq_pool_delete(struct snd_seq_pool **pool);

/* init memory */
int snd_sequencer_memory_init(void);
            
/* release event memory */
void snd_sequencer_memory_done(void);

/* polling */
int snd_seq_pool_poll_wait(struct snd_seq_pool *pool, struct file *file, poll_table *wait);


#endif
