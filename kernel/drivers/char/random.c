


#include <linux/utsname.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/string.h>
#include <linux/fcntl.h>
#include <linux/slab.h>
#include <linux/random.h>
#include <linux/poll.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/genhd.h>
#include <linux/interrupt.h>
#include <linux/mm.h>
#include <linux/spinlock.h>
#include <linux/percpu.h>
#include <linux/cryptohash.h>
#include <linux/fips.h>

#ifdef CONFIG_GENERIC_HARDIRQS
# include <linux/irq.h>
#endif

#include <asm/processor.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>

#define INPUT_POOL_WORDS 128
#define OUTPUT_POOL_WORDS 32
#define SEC_XFER_SIZE 512
#define EXTRACT_SIZE 10

static int random_read_wakeup_thresh = 64;

static int random_write_wakeup_thresh = 128;


static int trickle_thresh __read_mostly = INPUT_POOL_WORDS * 28;

static DEFINE_PER_CPU(int, trickle_count);

static struct poolinfo {
	int poolwords;
	int tap1, tap2, tap3, tap4, tap5;
} poolinfo_table[] = {
	/* x^128 + x^103 + x^76 + x^51 +x^25 + x + 1 -- 105 */
	{ 128,	103,	76,	51,	25,	1 },
	/* x^32 + x^26 + x^20 + x^14 + x^7 + x + 1 -- 15 */
	{ 32,	26,	20,	14,	7,	1 },
#if 0
	/* x^2048 + x^1638 + x^1231 + x^819 + x^411 + x + 1  -- 115 */
	{ 2048,	1638,	1231,	819,	411,	1 },

	/* x^1024 + x^817 + x^615 + x^412 + x^204 + x + 1 -- 290 */
	{ 1024,	817,	615,	412,	204,	1 },

	/* x^1024 + x^819 + x^616 + x^410 + x^207 + x^2 + 1 -- 115 */
	{ 1024,	819,	616,	410,	207,	2 },

	/* x^512 + x^411 + x^308 + x^208 + x^104 + x + 1 -- 225 */
	{ 512,	411,	308,	208,	104,	1 },

	/* x^512 + x^409 + x^307 + x^206 + x^102 + x^2 + 1 -- 95 */
	{ 512,	409,	307,	206,	102,	2 },
	/* x^512 + x^409 + x^309 + x^205 + x^103 + x^2 + 1 -- 95 */
	{ 512,	409,	309,	205,	103,	2 },

	/* x^256 + x^205 + x^155 + x^101 + x^52 + x + 1 -- 125 */
	{ 256,	205,	155,	101,	52,	1 },

	/* x^128 + x^103 + x^78 + x^51 + x^27 + x^2 + 1 -- 70 */
	{ 128,	103,	78,	51,	27,	2 },

	/* x^64 + x^52 + x^39 + x^26 + x^14 + x + 1 -- 15 */
	{ 64,	52,	39,	26,	14,	1 },
#endif
};

#define POOLBITS	poolwords*32
#define POOLBYTES	poolwords*4


static DECLARE_WAIT_QUEUE_HEAD(random_read_wait);
static DECLARE_WAIT_QUEUE_HEAD(random_write_wait);
static struct fasync_struct *fasync;

#if 0
static int debug;
module_param(debug, bool, 0644);
#define DEBUG_ENT(fmt, arg...) do { \
	if (debug) \
		printk(KERN_DEBUG "random %04d %04d %04d: " \
		fmt,\
		input_pool.entropy_count,\
		blocking_pool.entropy_count,\
		nonblocking_pool.entropy_count,\
		## arg); } while (0)
#else
#define DEBUG_ENT(fmt, arg...) do {} while (0)
#endif


struct entropy_store;
struct entropy_store {
	/* read-only data: */
	struct poolinfo *poolinfo;
	__u32 *pool;
	const char *name;
	int limit;
	struct entropy_store *pull;

	/* read-write data: */
	spinlock_t lock;
	unsigned add_ptr;
	int entropy_count;
	int input_rotate;
	__u8 last_data[EXTRACT_SIZE];
};

static __u32 input_pool_data[INPUT_POOL_WORDS];
static __u32 blocking_pool_data[OUTPUT_POOL_WORDS];
static __u32 nonblocking_pool_data[OUTPUT_POOL_WORDS];

static struct entropy_store input_pool = {
	.poolinfo = &poolinfo_table[0],
	.name = "input",
	.limit = 1,
	.lock = __SPIN_LOCK_UNLOCKED(&input_pool.lock),
	.pool = input_pool_data
};

static struct entropy_store blocking_pool = {
	.poolinfo = &poolinfo_table[1],
	.name = "blocking",
	.limit = 1,
	.pull = &input_pool,
	.lock = __SPIN_LOCK_UNLOCKED(&blocking_pool.lock),
	.pool = blocking_pool_data
};

static struct entropy_store nonblocking_pool = {
	.poolinfo = &poolinfo_table[1],
	.name = "nonblocking",
	.pull = &input_pool,
	.lock = __SPIN_LOCK_UNLOCKED(&nonblocking_pool.lock),
	.pool = nonblocking_pool_data
};

static void mix_pool_bytes_extract(struct entropy_store *r, const void *in,
				   int nbytes, __u8 out[64])
{
	static __u32 const twist_table[8] = {
		0x00000000, 0x3b6e20c8, 0x76dc4190, 0x4db26158,
		0xedb88320, 0xd6d6a3e8, 0x9b64c2b0, 0xa00ae278 };
	unsigned long i, j, tap1, tap2, tap3, tap4, tap5;
	int input_rotate;
	int wordmask = r->poolinfo->poolwords - 1;
	const char *bytes = in;
	__u32 w;
	unsigned long flags;

	/* Taps are constant, so we can load them without holding r->lock.  */
	tap1 = r->poolinfo->tap1;
	tap2 = r->poolinfo->tap2;
	tap3 = r->poolinfo->tap3;
	tap4 = r->poolinfo->tap4;
	tap5 = r->poolinfo->tap5;

	spin_lock_irqsave(&r->lock, flags);
	input_rotate = r->input_rotate;
	i = r->add_ptr;

	/* mix one byte at a time to simplify size handling and churn faster */
	while (nbytes--) {
		w = rol32(*bytes++, input_rotate & 31);
		i = (i - 1) & wordmask;

		/* XOR in the various taps */
		w ^= r->pool[i];
		w ^= r->pool[(i + tap1) & wordmask];
		w ^= r->pool[(i + tap2) & wordmask];
		w ^= r->pool[(i + tap3) & wordmask];
		w ^= r->pool[(i + tap4) & wordmask];
		w ^= r->pool[(i + tap5) & wordmask];

		/* Mix the result back in with a twist */
		r->pool[i] = (w >> 3) ^ twist_table[w & 7];

		/*
		 * Normally, we add 7 bits of rotation to the pool.
		 * At the beginning of the pool, add an extra 7 bits
		 * rotation, so that successive passes spread the
		 * input bits across the pool evenly.
		 */
		input_rotate += i ? 7 : 14;
	}

	r->input_rotate = input_rotate;
	r->add_ptr = i;

	if (out)
		for (j = 0; j < 16; j++)
			((__u32 *)out)[j] = r->pool[(i - j) & wordmask];

	spin_unlock_irqrestore(&r->lock, flags);
}

static void mix_pool_bytes(struct entropy_store *r, const void *in, int bytes)
{
       mix_pool_bytes_extract(r, in, bytes, NULL);
}

static void credit_entropy_bits(struct entropy_store *r, int nbits)
{
	unsigned long flags;
	int entropy_count;

	if (!nbits)
		return;

	spin_lock_irqsave(&r->lock, flags);

	DEBUG_ENT("added %d entropy credits to %s\n", nbits, r->name);
	entropy_count = r->entropy_count;
	entropy_count += nbits;
	if (entropy_count < 0) {
		DEBUG_ENT("negative entropy/overflow\n");
		entropy_count = 0;
	} else if (entropy_count > r->poolinfo->POOLBITS)
		entropy_count = r->poolinfo->POOLBITS;
	r->entropy_count = entropy_count;

	/* should we wake readers? */
	if (r == &input_pool && entropy_count >= random_read_wakeup_thresh) {
		wake_up_interruptible(&random_read_wait);
		kill_fasync(&fasync, SIGIO, POLL_IN);
	}
	spin_unlock_irqrestore(&r->lock, flags);
}


/* There is one of these per entropy source */
struct timer_rand_state {
	cycles_t last_time;
	long last_delta, last_delta2;
	unsigned dont_count_entropy:1;
};

#ifndef CONFIG_GENERIC_HARDIRQS

static struct timer_rand_state *irq_timer_state[NR_IRQS];

static struct timer_rand_state *get_timer_rand_state(unsigned int irq)
{
	return irq_timer_state[irq];
}

static void set_timer_rand_state(unsigned int irq,
				 struct timer_rand_state *state)
{
	irq_timer_state[irq] = state;
}

#else

static struct timer_rand_state *get_timer_rand_state(unsigned int irq)
{
	struct irq_desc *desc;

	desc = irq_to_desc(irq);

	return desc->timer_rand_state;
}

static void set_timer_rand_state(unsigned int irq,
				 struct timer_rand_state *state)
{
	struct irq_desc *desc;

	desc = irq_to_desc(irq);

	desc->timer_rand_state = state;
}
#endif

static struct timer_rand_state input_timer_state;

static void add_timer_randomness(struct timer_rand_state *state, unsigned num)
{
	struct {
		cycles_t cycles;
		long jiffies;
		unsigned num;
	} sample;
	long delta, delta2, delta3;

	preempt_disable();
	/* if over the trickle threshold, use only 1 in 4096 samples */
	if (input_pool.entropy_count > trickle_thresh &&
	    (__get_cpu_var(trickle_count)++ & 0xfff))
		goto out;

	sample.jiffies = jiffies;
	sample.cycles = get_cycles();
	sample.num = num;
	mix_pool_bytes(&input_pool, &sample, sizeof(sample));

	/*
	 * Calculate number of bits of randomness we probably added.
	 * We take into account the first, second and third-order deltas
	 * in order to make our estimate.
	 */

	if (!state->dont_count_entropy) {
		delta = sample.jiffies - state->last_time;
		state->last_time = sample.jiffies;

		delta2 = delta - state->last_delta;
		state->last_delta = delta;

		delta3 = delta2 - state->last_delta2;
		state->last_delta2 = delta2;

		if (delta < 0)
			delta = -delta;
		if (delta2 < 0)
			delta2 = -delta2;
		if (delta3 < 0)
			delta3 = -delta3;
		if (delta > delta2)
			delta = delta2;
		if (delta > delta3)
			delta = delta3;

		/*
		 * delta is now minimum absolute delta.
		 * Round down by 1 bit on general principles,
		 * and limit entropy entimate to 12 bits.
		 */
		credit_entropy_bits(&input_pool,
				    min_t(int, fls(delta>>1), 11));
	}
out:
	preempt_enable();
}

void add_input_randomness(unsigned int type, unsigned int code,
				 unsigned int value)
{
	static unsigned char last_value;

	/* ignore autorepeat and the like */
	if (value == last_value)
		return;

	DEBUG_ENT("input event\n");
	last_value = value;
	add_timer_randomness(&input_timer_state,
			     (type << 4) ^ code ^ (code >> 4) ^ value);
}
EXPORT_SYMBOL_GPL(add_input_randomness);

void add_interrupt_randomness(int irq)
{
	struct timer_rand_state *state;

	state = get_timer_rand_state(irq);

	if (state == NULL)
		return;

	DEBUG_ENT("irq event %d\n", irq);
	add_timer_randomness(state, 0x100 + irq);
}

#ifdef CONFIG_BLOCK
void add_disk_randomness(struct gendisk *disk)
{
	if (!disk || !disk->random)
		return;
	/* first major is 1, so we get >= 0x200 here */
	DEBUG_ENT("disk event %d:%d\n",
		  MAJOR(disk_devt(disk)), MINOR(disk_devt(disk)));

	add_timer_randomness(disk->random, 0x100 + disk_devt(disk));
}
#endif


static ssize_t extract_entropy(struct entropy_store *r, void *buf,
			       size_t nbytes, int min, int rsvd);

static void xfer_secondary_pool(struct entropy_store *r, size_t nbytes)
{
	__u32 tmp[OUTPUT_POOL_WORDS];

	if (r->pull && r->entropy_count < nbytes * 8 &&
	    r->entropy_count < r->poolinfo->POOLBITS) {
		/* If we're limited, always leave two wakeup worth's BITS */
		int rsvd = r->limit ? 0 : random_read_wakeup_thresh/4;
		int bytes = nbytes;

		/* pull at least as many as BYTES as wakeup BITS */
		bytes = max_t(int, bytes, random_read_wakeup_thresh / 8);
		/* but never more than the buffer size */
		bytes = min_t(int, bytes, sizeof(tmp));

		DEBUG_ENT("going to reseed %s with %d bits "
			  "(%d of %d requested)\n",
			  r->name, bytes * 8, nbytes * 8, r->entropy_count);

		bytes = extract_entropy(r->pull, tmp, bytes,
					random_read_wakeup_thresh / 8, rsvd);
		mix_pool_bytes(r, tmp, bytes);
		credit_entropy_bits(r, bytes*8);
	}
}


static size_t account(struct entropy_store *r, size_t nbytes, int min,
		      int reserved)
{
	unsigned long flags;

	/* Hold lock while accounting */
	spin_lock_irqsave(&r->lock, flags);

	BUG_ON(r->entropy_count > r->poolinfo->POOLBITS);
	DEBUG_ENT("trying to extract %d bits from %s\n",
		  nbytes * 8, r->name);

	/* Can we pull enough? */
	if (r->entropy_count / 8 < min + reserved) {
		nbytes = 0;
	} else {
		/* If limited, never pull more than available */
		if (r->limit && nbytes + reserved >= r->entropy_count / 8)
			nbytes = r->entropy_count/8 - reserved;

		if (r->entropy_count / 8 >= nbytes + reserved)
			r->entropy_count -= nbytes*8;
		else
			r->entropy_count = reserved;

		if (r->entropy_count < random_write_wakeup_thresh) {
			wake_up_interruptible(&random_write_wait);
			kill_fasync(&fasync, SIGIO, POLL_OUT);
		}
	}

	DEBUG_ENT("debiting %d entropy credits from %s%s\n",
		  nbytes * 8, r->name, r->limit ? "" : " (unlimited)");

	spin_unlock_irqrestore(&r->lock, flags);

	return nbytes;
}

static void extract_buf(struct entropy_store *r, __u8 *out)
{
	int i;
	__u32 hash[5], workspace[SHA_WORKSPACE_WORDS];
	__u8 extract[64];

	/* Generate a hash across the pool, 16 words (512 bits) at a time */
	sha_init(hash);
	for (i = 0; i < r->poolinfo->poolwords; i += 16)
		sha_transform(hash, (__u8 *)(r->pool + i), workspace);

	/*
	 * We mix the hash back into the pool to prevent backtracking
	 * attacks (where the attacker knows the state of the pool
	 * plus the current outputs, and attempts to find previous
	 * ouputs), unless the hash function can be inverted. By
	 * mixing at least a SHA1 worth of hash data back, we make
	 * brute-forcing the feedback as hard as brute-forcing the
	 * hash.
	 */
	mix_pool_bytes_extract(r, hash, sizeof(hash), extract);

	/*
	 * To avoid duplicates, we atomically extract a portion of the
	 * pool while mixing, and hash one final time.
	 */
	sha_transform(hash, extract, workspace);
	memset(extract, 0, sizeof(extract));
	memset(workspace, 0, sizeof(workspace));

	/*
	 * In case the hash function has some recognizable output
	 * pattern, we fold it in half. Thus, we always feed back
	 * twice as much data as we output.
	 */
	hash[0] ^= hash[3];
	hash[1] ^= hash[4];
	hash[2] ^= rol32(hash[2], 16);
	memcpy(out, hash, EXTRACT_SIZE);
	memset(hash, 0, sizeof(hash));
}

static ssize_t extract_entropy(struct entropy_store *r, void *buf,
			       size_t nbytes, int min, int reserved)
{
	ssize_t ret = 0, i;
	__u8 tmp[EXTRACT_SIZE];
	unsigned long flags;

	xfer_secondary_pool(r, nbytes);
	nbytes = account(r, nbytes, min, reserved);

	while (nbytes) {
		extract_buf(r, tmp);

		if (fips_enabled) {
			spin_lock_irqsave(&r->lock, flags);
			if (!memcmp(tmp, r->last_data, EXTRACT_SIZE))
				panic("Hardware RNG duplicated output!\n");
			memcpy(r->last_data, tmp, EXTRACT_SIZE);
			spin_unlock_irqrestore(&r->lock, flags);
		}
		i = min_t(int, nbytes, EXTRACT_SIZE);
		memcpy(buf, tmp, i);
		nbytes -= i;
		buf += i;
		ret += i;
	}

	/* Wipe data just returned from memory */
	memset(tmp, 0, sizeof(tmp));

	return ret;
}

static ssize_t extract_entropy_user(struct entropy_store *r, void __user *buf,
				    size_t nbytes)
{
	ssize_t ret = 0, i;
	__u8 tmp[EXTRACT_SIZE];

	xfer_secondary_pool(r, nbytes);
	nbytes = account(r, nbytes, 0, 0);

	while (nbytes) {
		if (need_resched()) {
			if (signal_pending(current)) {
				if (ret == 0)
					ret = -ERESTARTSYS;
				break;
			}
			schedule();
		}

		extract_buf(r, tmp);
		i = min_t(int, nbytes, EXTRACT_SIZE);
		if (copy_to_user(buf, tmp, i)) {
			ret = -EFAULT;
			break;
		}

		nbytes -= i;
		buf += i;
		ret += i;
	}

	/* Wipe data just returned from memory */
	memset(tmp, 0, sizeof(tmp));

	return ret;
}

void get_random_bytes(void *buf, int nbytes)
{
	extract_entropy(&nonblocking_pool, buf, nbytes, 0, 0);
}
EXPORT_SYMBOL(get_random_bytes);

static void init_std_data(struct entropy_store *r)
{
	ktime_t now;
	unsigned long flags;

	spin_lock_irqsave(&r->lock, flags);
	r->entropy_count = 0;
	spin_unlock_irqrestore(&r->lock, flags);

	now = ktime_get_real();
	mix_pool_bytes(r, &now, sizeof(now));
	mix_pool_bytes(r, utsname(), sizeof(*(utsname())));
}

static int rand_initialize(void)
{
	init_std_data(&input_pool);
	init_std_data(&blocking_pool);
	init_std_data(&nonblocking_pool);
	return 0;
}
module_init(rand_initialize);

void rand_initialize_irq(int irq)
{
	struct timer_rand_state *state;

	state = get_timer_rand_state(irq);

	if (state)
		return;

	/*
	 * If kzalloc returns null, we just won't use that entropy
	 * source.
	 */
	state = kzalloc(sizeof(struct timer_rand_state), GFP_KERNEL);
	if (state)
		set_timer_rand_state(irq, state);
}

#ifdef CONFIG_BLOCK
void rand_initialize_disk(struct gendisk *disk)
{
	struct timer_rand_state *state;

	/*
	 * If kzalloc returns null, we just won't use that entropy
	 * source.
	 */
	state = kzalloc(sizeof(struct timer_rand_state), GFP_KERNEL);
	if (state)
		disk->random = state;
}
#endif

static ssize_t
random_read(struct file *file, char __user *buf, size_t nbytes, loff_t *ppos)
{
	ssize_t n, retval = 0, count = 0;

	if (nbytes == 0)
		return 0;

	while (nbytes > 0) {
		n = nbytes;
		if (n > SEC_XFER_SIZE)
			n = SEC_XFER_SIZE;

		DEBUG_ENT("reading %d bits\n", n*8);

		n = extract_entropy_user(&blocking_pool, buf, n);

		DEBUG_ENT("read got %d bits (%d still needed)\n",
			  n*8, (nbytes-n)*8);

		if (n == 0) {
			if (file->f_flags & O_NONBLOCK) {
				retval = -EAGAIN;
				break;
			}

			DEBUG_ENT("sleeping?\n");

			wait_event_interruptible(random_read_wait,
				input_pool.entropy_count >=
						 random_read_wakeup_thresh);

			DEBUG_ENT("awake\n");

			if (signal_pending(current)) {
				retval = -ERESTARTSYS;
				break;
			}

			continue;
		}

		if (n < 0) {
			retval = n;
			break;
		}
		count += n;
		buf += n;
		nbytes -= n;
		break;		/* This break makes the device work */
				/* like a named pipe */
	}

	return (count ? count : retval);
}

static ssize_t
urandom_read(struct file *file, char __user *buf, size_t nbytes, loff_t *ppos)
{
	return extract_entropy_user(&nonblocking_pool, buf, nbytes);
}

static unsigned int
random_poll(struct file *file, poll_table * wait)
{
	unsigned int mask;

	poll_wait(file, &random_read_wait, wait);
	poll_wait(file, &random_write_wait, wait);
	mask = 0;
	if (input_pool.entropy_count >= random_read_wakeup_thresh)
		mask |= POLLIN | POLLRDNORM;
	if (input_pool.entropy_count < random_write_wakeup_thresh)
		mask |= POLLOUT | POLLWRNORM;
	return mask;
}

static int
write_pool(struct entropy_store *r, const char __user *buffer, size_t count)
{
	size_t bytes;
	__u32 buf[16];
	const char __user *p = buffer;

	while (count > 0) {
		bytes = min(count, sizeof(buf));
		if (copy_from_user(&buf, p, bytes))
			return -EFAULT;

		count -= bytes;
		p += bytes;

		mix_pool_bytes(r, buf, bytes);
		cond_resched();
	}

	return 0;
}

static ssize_t random_write(struct file *file, const char __user *buffer,
			    size_t count, loff_t *ppos)
{
	size_t ret;

	ret = write_pool(&blocking_pool, buffer, count);
	if (ret)
		return ret;
	ret = write_pool(&nonblocking_pool, buffer, count);
	if (ret)
		return ret;

	return (ssize_t)count;
}

static long random_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	int size, ent_count;
	int __user *p = (int __user *)arg;
	int retval;

	switch (cmd) {
	case RNDGETENTCNT:
		/* inherently racy, no point locking */
		if (put_user(input_pool.entropy_count, p))
			return -EFAULT;
		return 0;
	case RNDADDTOENTCNT:
		if (!capable(CAP_SYS_ADMIN))
			return -EPERM;
		if (get_user(ent_count, p))
			return -EFAULT;
		credit_entropy_bits(&input_pool, ent_count);
		return 0;
	case RNDADDENTROPY:
		if (!capable(CAP_SYS_ADMIN))
			return -EPERM;
		if (get_user(ent_count, p++))
			return -EFAULT;
		if (ent_count < 0)
			return -EINVAL;
		if (get_user(size, p++))
			return -EFAULT;
		retval = write_pool(&input_pool, (const char __user *)p,
				    size);
		if (retval < 0)
			return retval;
		credit_entropy_bits(&input_pool, ent_count);
		return 0;
	case RNDZAPENTCNT:
	case RNDCLEARPOOL:
		/* Clear the entropy pool counters. */
		if (!capable(CAP_SYS_ADMIN))
			return -EPERM;
		rand_initialize();
		return 0;
	default:
		return -EINVAL;
	}
}

static int random_fasync(int fd, struct file *filp, int on)
{
	return fasync_helper(fd, filp, on, &fasync);
}

const struct file_operations random_fops = {
	.read  = random_read,
	.write = random_write,
	.poll  = random_poll,
	.unlocked_ioctl = random_ioctl,
	.fasync = random_fasync,
};

const struct file_operations urandom_fops = {
	.read  = urandom_read,
	.write = random_write,
	.unlocked_ioctl = random_ioctl,
	.fasync = random_fasync,
};


void generate_random_uuid(unsigned char uuid_out[16])
{
	get_random_bytes(uuid_out, 16);
	/* Set UUID version to 4 --- truly random generation */
	uuid_out[6] = (uuid_out[6] & 0x0F) | 0x40;
	/* Set the UUID variant to DCE */
	uuid_out[8] = (uuid_out[8] & 0x3F) | 0x80;
}
EXPORT_SYMBOL(generate_random_uuid);


#ifdef CONFIG_SYSCTL

#include <linux/sysctl.h>

static int min_read_thresh = 8, min_write_thresh;
static int max_read_thresh = INPUT_POOL_WORDS * 32;
static int max_write_thresh = INPUT_POOL_WORDS * 32;
static char sysctl_bootid[16];

static int proc_do_uuid(ctl_table *table, int write,
			void __user *buffer, size_t *lenp, loff_t *ppos)
{
	ctl_table fake_table;
	unsigned char buf[64], tmp_uuid[16], *uuid;

	uuid = table->data;
	if (!uuid) {
		uuid = tmp_uuid;
		uuid[8] = 0;
	}
	if (uuid[8] == 0)
		generate_random_uuid(uuid);

	sprintf(buf, "%pU", uuid);

	fake_table.data = buf;
	fake_table.maxlen = sizeof(buf);

	return proc_dostring(&fake_table, write, buffer, lenp, ppos);
}

static int sysctl_poolsize = INPUT_POOL_WORDS * 32;
ctl_table random_table[] = {
	{
		.procname	= "poolsize",
		.data		= &sysctl_poolsize,
		.maxlen		= sizeof(int),
		.mode		= 0444,
		.proc_handler	= proc_dointvec,
	},
	{
		.procname	= "entropy_avail",
		.maxlen		= sizeof(int),
		.mode		= 0444,
		.proc_handler	= proc_dointvec,
		.data		= &input_pool.entropy_count,
	},
	{
		.procname	= "read_wakeup_threshold",
		.data		= &random_read_wakeup_thresh,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec_minmax,
		.extra1		= &min_read_thresh,
		.extra2		= &max_read_thresh,
	},
	{
		.procname	= "write_wakeup_threshold",
		.data		= &random_write_wakeup_thresh,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec_minmax,
		.extra1		= &min_write_thresh,
		.extra2		= &max_write_thresh,
	},
	{
		.procname	= "boot_id",
		.data		= &sysctl_bootid,
		.maxlen		= 16,
		.mode		= 0444,
		.proc_handler	= proc_do_uuid,
	},
	{
		.procname	= "uuid",
		.maxlen		= 16,
		.mode		= 0444,
		.proc_handler	= proc_do_uuid,
	},
	{ }
};
#endif 	/* CONFIG_SYSCTL */



/* F, G and H are basic MD4 functions: selection, majority, parity */
#define F(x, y, z) ((z) ^ ((x) & ((y) ^ (z))))
#define G(x, y, z) (((x) & (y)) + (((x) ^ (y)) & (z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))

#define ROUND(f, a, b, c, d, x, s)	\
	(a += f(b, c, d) + x, a = (a << s) | (a >> (32 - s)))
#define K1 0
#define K2 013240474631UL
#define K3 015666365641UL

#if defined(CONFIG_IPV6) || defined(CONFIG_IPV6_MODULE)

static __u32 twothirdsMD4Transform(__u32 const buf[4], __u32 const in[12])
{
	__u32 a = buf[0], b = buf[1], c = buf[2], d = buf[3];

	/* Round 1 */
	ROUND(F, a, b, c, d, in[ 0] + K1,  3);
	ROUND(F, d, a, b, c, in[ 1] + K1,  7);
	ROUND(F, c, d, a, b, in[ 2] + K1, 11);
	ROUND(F, b, c, d, a, in[ 3] + K1, 19);
	ROUND(F, a, b, c, d, in[ 4] + K1,  3);
	ROUND(F, d, a, b, c, in[ 5] + K1,  7);
	ROUND(F, c, d, a, b, in[ 6] + K1, 11);
	ROUND(F, b, c, d, a, in[ 7] + K1, 19);
	ROUND(F, a, b, c, d, in[ 8] + K1,  3);
	ROUND(F, d, a, b, c, in[ 9] + K1,  7);
	ROUND(F, c, d, a, b, in[10] + K1, 11);
	ROUND(F, b, c, d, a, in[11] + K1, 19);

	/* Round 2 */
	ROUND(G, a, b, c, d, in[ 1] + K2,  3);
	ROUND(G, d, a, b, c, in[ 3] + K2,  5);
	ROUND(G, c, d, a, b, in[ 5] + K2,  9);
	ROUND(G, b, c, d, a, in[ 7] + K2, 13);
	ROUND(G, a, b, c, d, in[ 9] + K2,  3);
	ROUND(G, d, a, b, c, in[11] + K2,  5);
	ROUND(G, c, d, a, b, in[ 0] + K2,  9);
	ROUND(G, b, c, d, a, in[ 2] + K2, 13);
	ROUND(G, a, b, c, d, in[ 4] + K2,  3);
	ROUND(G, d, a, b, c, in[ 6] + K2,  5);
	ROUND(G, c, d, a, b, in[ 8] + K2,  9);
	ROUND(G, b, c, d, a, in[10] + K2, 13);

	/* Round 3 */
	ROUND(H, a, b, c, d, in[ 3] + K3,  3);
	ROUND(H, d, a, b, c, in[ 7] + K3,  9);
	ROUND(H, c, d, a, b, in[11] + K3, 11);
	ROUND(H, b, c, d, a, in[ 2] + K3, 15);
	ROUND(H, a, b, c, d, in[ 6] + K3,  3);
	ROUND(H, d, a, b, c, in[10] + K3,  9);
	ROUND(H, c, d, a, b, in[ 1] + K3, 11);
	ROUND(H, b, c, d, a, in[ 5] + K3, 15);
	ROUND(H, a, b, c, d, in[ 9] + K3,  3);
	ROUND(H, d, a, b, c, in[ 0] + K3,  9);
	ROUND(H, c, d, a, b, in[ 4] + K3, 11);
	ROUND(H, b, c, d, a, in[ 8] + K3, 15);

	return buf[1] + b; /* "most hashed" word */
	/* Alternative: return sum of all words? */
}
#endif

#undef ROUND
#undef F
#undef G
#undef H
#undef K1
#undef K2
#undef K3

/* This should not be decreased so low that ISNs wrap too fast. */
#define REKEY_INTERVAL (300 * HZ)
#define COUNT_BITS 8
#define COUNT_MASK ((1 << COUNT_BITS) - 1)
#define HASH_BITS 24
#define HASH_MASK ((1 << HASH_BITS) - 1)

static struct keydata {
	__u32 count; /* already shifted to the final position */
	__u32 secret[12];
} ____cacheline_aligned ip_keydata[2];

static unsigned int ip_cnt;

static void rekey_seq_generator(struct work_struct *work);

static DECLARE_DELAYED_WORK(rekey_work, rekey_seq_generator);

static void rekey_seq_generator(struct work_struct *work)
{
	struct keydata *keyptr = &ip_keydata[1 ^ (ip_cnt & 1)];

	get_random_bytes(keyptr->secret, sizeof(keyptr->secret));
	keyptr->count = (ip_cnt & COUNT_MASK) << HASH_BITS;
	smp_wmb();
	ip_cnt++;
	schedule_delayed_work(&rekey_work,
			      round_jiffies_relative(REKEY_INTERVAL));
}

static inline struct keydata *get_keyptr(void)
{
	struct keydata *keyptr = &ip_keydata[ip_cnt & 1];

	smp_rmb();

	return keyptr;
}

static __init int seqgen_init(void)
{
	rekey_seq_generator(NULL);
	return 0;
}
late_initcall(seqgen_init);

#if defined(CONFIG_IPV6) || defined(CONFIG_IPV6_MODULE)
__u32 secure_tcpv6_sequence_number(__be32 *saddr, __be32 *daddr,
				   __be16 sport, __be16 dport)
{
	__u32 seq;
	__u32 hash[12];
	struct keydata *keyptr = get_keyptr();

	/* The procedure is the same as for IPv4, but addresses are longer.
	 * Thus we must use twothirdsMD4Transform.
	 */

	memcpy(hash, saddr, 16);
	hash[4] = ((__force u16)sport << 16) + (__force u16)dport;
	memcpy(&hash[5], keyptr->secret, sizeof(__u32) * 7);

	seq = twothirdsMD4Transform((const __u32 *)daddr, hash) & HASH_MASK;
	seq += keyptr->count;

	seq += ktime_to_ns(ktime_get_real());

	return seq;
}
EXPORT_SYMBOL(secure_tcpv6_sequence_number);
#endif

__u32 secure_ip_id(__be32 daddr)
{
	struct keydata *keyptr;
	__u32 hash[4];

	keyptr = get_keyptr();

	/*
	 *  Pick a unique starting offset for each IP destination.
	 *  The dest ip address is placed in the starting vector,
	 *  which is then hashed with random data.
	 */
	hash[0] = (__force __u32)daddr;
	hash[1] = keyptr->secret[9];
	hash[2] = keyptr->secret[10];
	hash[3] = keyptr->secret[11];

	return half_md4_transform(hash, keyptr->secret);
}

#ifdef CONFIG_INET

__u32 secure_tcp_sequence_number(__be32 saddr, __be32 daddr,
				 __be16 sport, __be16 dport)
{
	__u32 seq;
	__u32 hash[4];
	struct keydata *keyptr = get_keyptr();

	/*
	 *  Pick a unique starting offset for each TCP connection endpoints
	 *  (saddr, daddr, sport, dport).
	 *  Note that the words are placed into the starting vector, which is
	 *  then mixed with a partial MD4 over random data.
	 */
	hash[0] = (__force u32)saddr;
	hash[1] = (__force u32)daddr;
	hash[2] = ((__force u16)sport << 16) + (__force u16)dport;
	hash[3] = keyptr->secret[11];

	seq = half_md4_transform(hash, keyptr->secret) & HASH_MASK;
	seq += keyptr->count;
	/*
	 *	As close as possible to RFC 793, which
	 *	suggests using a 250 kHz clock.
	 *	Further reading shows this assumes 2 Mb/s networks.
	 *	For 10 Mb/s Ethernet, a 1 MHz clock is appropriate.
	 *	For 10 Gb/s Ethernet, a 1 GHz clock should be ok, but
	 *	we also need to limit the resolution so that the u32 seq
	 *	overlaps less than one time per MSL (2 minutes).
	 *	Choosing a clock of 64 ns period is OK. (period of 274 s)
	 */
	seq += ktime_to_ns(ktime_get_real()) >> 6;

	return seq;
}

/* Generate secure starting point for ephemeral IPV4 transport port search */
u32 secure_ipv4_port_ephemeral(__be32 saddr, __be32 daddr, __be16 dport)
{
	struct keydata *keyptr = get_keyptr();
	u32 hash[4];

	/*
	 *  Pick a unique starting offset for each ephemeral port search
	 *  (saddr, daddr, dport) and 48bits of random data.
	 */
	hash[0] = (__force u32)saddr;
	hash[1] = (__force u32)daddr;
	hash[2] = (__force u32)dport ^ keyptr->secret[10];
	hash[3] = keyptr->secret[11];

	return half_md4_transform(hash, keyptr->secret);
}
EXPORT_SYMBOL_GPL(secure_ipv4_port_ephemeral);

#if defined(CONFIG_IPV6) || defined(CONFIG_IPV6_MODULE)
u32 secure_ipv6_port_ephemeral(const __be32 *saddr, const __be32 *daddr,
			       __be16 dport)
{
	struct keydata *keyptr = get_keyptr();
	u32 hash[12];

	memcpy(hash, saddr, 16);
	hash[4] = (__force u32)dport;
	memcpy(&hash[5], keyptr->secret, sizeof(__u32) * 7);

	return twothirdsMD4Transform((const __u32 *)daddr, hash);
}
#endif

#if defined(CONFIG_IP_DCCP) || defined(CONFIG_IP_DCCP_MODULE)
u64 secure_dccp_sequence_number(__be32 saddr, __be32 daddr,
				__be16 sport, __be16 dport)
{
	u64 seq;
	__u32 hash[4];
	struct keydata *keyptr = get_keyptr();

	hash[0] = (__force u32)saddr;
	hash[1] = (__force u32)daddr;
	hash[2] = ((__force u16)sport << 16) + (__force u16)dport;
	hash[3] = keyptr->secret[11];

	seq = half_md4_transform(hash, keyptr->secret);
	seq |= ((u64)keyptr->count) << (32 - HASH_BITS);

	seq += ktime_to_ns(ktime_get_real());
	seq &= (1ull << 48) - 1;

	return seq;
}
EXPORT_SYMBOL(secure_dccp_sequence_number);
#endif

#endif /* CONFIG_INET */


DEFINE_PER_CPU(__u32 [4], get_random_int_hash);
unsigned int get_random_int(void)
{
	struct keydata *keyptr;
	__u32 *hash = get_cpu_var(get_random_int_hash);
	int ret;

	keyptr = get_keyptr();
	hash[0] += current->pid + jiffies + get_cycles();

	ret = half_md4_transform(hash, keyptr->secret);
	put_cpu_var(get_random_int_hash);

	return ret;
}

unsigned long
randomize_range(unsigned long start, unsigned long end, unsigned long len)
{
	unsigned long range = end - len - start;

	if (end <= start + len)
		return 0;
	return PAGE_ALIGN(get_random_int() % range + start);
}
