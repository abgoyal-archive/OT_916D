


#define PF_VERSION      "1.04"
#define PF_MAJOR	47
#define PF_NAME		"pf"
#define PF_UNITS	4


static int verbose = 0;
static int major = PF_MAJOR;
static char *name = PF_NAME;
static int cluster = 64;
static int nice = 0;
static int disable = 0;

static int drive0[7] = { 0, 0, 0, -1, -1, -1, -1 };
static int drive1[7] = { 0, 0, 0, -1, -1, -1, -1 };
static int drive2[7] = { 0, 0, 0, -1, -1, -1, -1 };
static int drive3[7] = { 0, 0, 0, -1, -1, -1, -1 };

static int (*drives[4])[7] = {&drive0, &drive1, &drive2, &drive3};
static int pf_drive_count;

enum {D_PRT, D_PRO, D_UNI, D_MOD, D_SLV, D_LUN, D_DLY};

/* end of parameters */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/hdreg.h>
#include <linux/cdrom.h>
#include <linux/spinlock.h>
#include <linux/blkdev.h>
#include <linux/blkpg.h>
#include <asm/uaccess.h>

static DEFINE_SPINLOCK(pf_spin_lock);

module_param(verbose, bool, 0644);
module_param(major, int, 0);
module_param(name, charp, 0);
module_param(cluster, int, 0);
module_param(nice, int, 0);
module_param_array(drive0, int, NULL, 0);
module_param_array(drive1, int, NULL, 0);
module_param_array(drive2, int, NULL, 0);
module_param_array(drive3, int, NULL, 0);

#include "paride.h"
#include "pseudo.h"

/* constants for faking geometry numbers */

#define PF_FD_MAX	8192	/* use FD geometry under this size */
#define PF_FD_HDS	2
#define PF_FD_SPT	18
#define PF_HD_HDS	64
#define PF_HD_SPT	32

#define PF_MAX_RETRIES  5
#define PF_TMO          800	/* interrupt timeout in jiffies */
#define PF_SPIN_DEL     50	/* spin delay in micro-seconds  */

#define PF_SPIN         (1000000*PF_TMO)/(HZ*PF_SPIN_DEL)

#define STAT_ERR        0x00001
#define STAT_INDEX      0x00002
#define STAT_ECC        0x00004
#define STAT_DRQ        0x00008
#define STAT_SEEK       0x00010
#define STAT_WRERR      0x00020
#define STAT_READY      0x00040
#define STAT_BUSY       0x00080

#define ATAPI_REQ_SENSE		0x03
#define ATAPI_LOCK		0x1e
#define ATAPI_DOOR		0x1b
#define ATAPI_MODE_SENSE	0x5a
#define ATAPI_CAPACITY		0x25
#define ATAPI_IDENTIFY		0x12
#define ATAPI_READ_10		0x28
#define ATAPI_WRITE_10		0x2a

static int pf_open(struct block_device *bdev, fmode_t mode);
static void do_pf_request(struct request_queue * q);
static int pf_ioctl(struct block_device *bdev, fmode_t mode,
		    unsigned int cmd, unsigned long arg);
static int pf_getgeo(struct block_device *bdev, struct hd_geometry *geo);

static int pf_release(struct gendisk *disk, fmode_t mode);

static int pf_detect(void);
static void do_pf_read(void);
static void do_pf_read_start(void);
static void do_pf_write(void);
static void do_pf_write_start(void);
static void do_pf_read_drq(void);
static void do_pf_write_done(void);

#define PF_NM           0
#define PF_RO           1
#define PF_RW           2

#define PF_NAMELEN      8

struct pf_unit {
	struct pi_adapter pia;	/* interface to paride layer */
	struct pi_adapter *pi;
	int removable;		/* removable media device  ?  */
	int media_status;	/* media present ?  WP ? */
	int drive;		/* drive */
	int lun;
	int access;		/* count of active opens ... */
	int present;		/* device present ? */
	char name[PF_NAMELEN];	/* pf0, pf1, ... */
	struct gendisk *disk;
};

static struct pf_unit units[PF_UNITS];

static int pf_identify(struct pf_unit *pf);
static void pf_lock(struct pf_unit *pf, int func);
static void pf_eject(struct pf_unit *pf);
static int pf_check_media(struct gendisk *disk);

static char pf_scratch[512];	/* scratch block buffer */


static int pf_retries = 0;	/* i/o error retry count */
static int pf_busy = 0;		/* request being processed ? */
static struct request *pf_req;	/* current request */
static int pf_block;		/* address of next requested block */
static int pf_count;		/* number of blocks still to do */
static int pf_run;		/* sectors in current cluster */
static int pf_cmd;		/* current command READ/WRITE */
static struct pf_unit *pf_current;/* unit of current request */
static int pf_mask;		/* stopper for pseudo-int */
static char *pf_buf;		/* buffer for request in progress */

/* kernel glue structures */

static const struct block_device_operations pf_fops = {
	.owner		= THIS_MODULE,
	.open		= pf_open,
	.release	= pf_release,
	.locked_ioctl	= pf_ioctl,
	.getgeo		= pf_getgeo,
	.media_changed	= pf_check_media,
};

static void __init pf_init_units(void)
{
	struct pf_unit *pf;
	int unit;

	pf_drive_count = 0;
	for (unit = 0, pf = units; unit < PF_UNITS; unit++, pf++) {
		struct gendisk *disk = alloc_disk(1);
		if (!disk)
			continue;
		pf->disk = disk;
		pf->pi = &pf->pia;
		pf->media_status = PF_NM;
		pf->drive = (*drives[unit])[D_SLV];
		pf->lun = (*drives[unit])[D_LUN];
		snprintf(pf->name, PF_NAMELEN, "%s%d", name, unit);
		disk->major = major;
		disk->first_minor = unit;
		strcpy(disk->disk_name, pf->name);
		disk->fops = &pf_fops;
		if (!(*drives[unit])[D_PRT])
			pf_drive_count++;
	}
}

static int pf_open(struct block_device *bdev, fmode_t mode)
{
	struct pf_unit *pf = bdev->bd_disk->private_data;

	pf_identify(pf);

	if (pf->media_status == PF_NM)
		return -ENODEV;

	if ((pf->media_status == PF_RO) && (mode & FMODE_WRITE))
		return -EROFS;

	pf->access++;
	if (pf->removable)
		pf_lock(pf, 1);

	return 0;
}

static int pf_getgeo(struct block_device *bdev, struct hd_geometry *geo)
{
	struct pf_unit *pf = bdev->bd_disk->private_data;
	sector_t capacity = get_capacity(pf->disk);

	if (capacity < PF_FD_MAX) {
		geo->cylinders = sector_div(capacity, PF_FD_HDS * PF_FD_SPT);
		geo->heads = PF_FD_HDS;
		geo->sectors = PF_FD_SPT;
	} else {
		geo->cylinders = sector_div(capacity, PF_HD_HDS * PF_HD_SPT);
		geo->heads = PF_HD_HDS;
		geo->sectors = PF_HD_SPT;
	}

	return 0;
}

static int pf_ioctl(struct block_device *bdev, fmode_t mode, unsigned int cmd, unsigned long arg)
{
	struct pf_unit *pf = bdev->bd_disk->private_data;

	if (cmd != CDROMEJECT)
		return -EINVAL;

	if (pf->access != 1)
		return -EBUSY;
	pf_eject(pf);
	return 0;
}

static int pf_release(struct gendisk *disk, fmode_t mode)
{
	struct pf_unit *pf = disk->private_data;

	if (pf->access <= 0)
		return -EINVAL;

	pf->access--;

	if (!pf->access && pf->removable)
		pf_lock(pf, 0);

	return 0;

}

static int pf_check_media(struct gendisk *disk)
{
	return 1;
}

static inline int status_reg(struct pf_unit *pf)
{
	return pi_read_regr(pf->pi, 1, 6);
}

static inline int read_reg(struct pf_unit *pf, int reg)
{
	return pi_read_regr(pf->pi, 0, reg);
}

static inline void write_reg(struct pf_unit *pf, int reg, int val)
{
	pi_write_regr(pf->pi, 0, reg, val);
}

static int pf_wait(struct pf_unit *pf, int go, int stop, char *fun, char *msg)
{
	int j, r, e, s, p;

	j = 0;
	while ((((r = status_reg(pf)) & go) || (stop && (!(r & stop))))
	       && (j++ < PF_SPIN))
		udelay(PF_SPIN_DEL);

	if ((r & (STAT_ERR & stop)) || (j > PF_SPIN)) {
		s = read_reg(pf, 7);
		e = read_reg(pf, 1);
		p = read_reg(pf, 2);
		if (j > PF_SPIN)
			e |= 0x100;
		if (fun)
			printk("%s: %s %s: alt=0x%x stat=0x%x err=0x%x"
			       " loop=%d phase=%d\n",
			       pf->name, fun, msg, r, s, e, j, p);
		return (e << 8) + s;
	}
	return 0;
}

static int pf_command(struct pf_unit *pf, char *cmd, int dlen, char *fun)
{
	pi_connect(pf->pi);

	write_reg(pf, 6, 0xa0+0x10*pf->drive);

	if (pf_wait(pf, STAT_BUSY | STAT_DRQ, 0, fun, "before command")) {
		pi_disconnect(pf->pi);
		return -1;
	}

	write_reg(pf, 4, dlen % 256);
	write_reg(pf, 5, dlen / 256);
	write_reg(pf, 7, 0xa0);	/* ATAPI packet command */

	if (pf_wait(pf, STAT_BUSY, STAT_DRQ, fun, "command DRQ")) {
		pi_disconnect(pf->pi);
		return -1;
	}

	if (read_reg(pf, 2) != 1) {
		printk("%s: %s: command phase error\n", pf->name, fun);
		pi_disconnect(pf->pi);
		return -1;
	}

	pi_write_block(pf->pi, cmd, 12);

	return 0;
}

static int pf_completion(struct pf_unit *pf, char *buf, char *fun)
{
	int r, s, n;

	r = pf_wait(pf, STAT_BUSY, STAT_DRQ | STAT_READY | STAT_ERR,
		    fun, "completion");

	if ((read_reg(pf, 2) & 2) && (read_reg(pf, 7) & STAT_DRQ)) {
		n = (((read_reg(pf, 4) + 256 * read_reg(pf, 5)) +
		      3) & 0xfffc);
		pi_read_block(pf->pi, buf, n);
	}

	s = pf_wait(pf, STAT_BUSY, STAT_READY | STAT_ERR, fun, "data done");

	pi_disconnect(pf->pi);

	return (r ? r : s);
}

static void pf_req_sense(struct pf_unit *pf, int quiet)
{
	char rs_cmd[12] =
	    { ATAPI_REQ_SENSE, pf->lun << 5, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0 };
	char buf[16];
	int r;

	r = pf_command(pf, rs_cmd, 16, "Request sense");
	mdelay(1);
	if (!r)
		pf_completion(pf, buf, "Request sense");

	if ((!r) && (!quiet))
		printk("%s: Sense key: %x, ASC: %x, ASQ: %x\n",
		       pf->name, buf[2] & 0xf, buf[12], buf[13]);
}

static int pf_atapi(struct pf_unit *pf, char *cmd, int dlen, char *buf, char *fun)
{
	int r;

	r = pf_command(pf, cmd, dlen, fun);
	mdelay(1);
	if (!r)
		r = pf_completion(pf, buf, fun);
	if (r)
		pf_req_sense(pf, !fun);

	return r;
}

static void pf_lock(struct pf_unit *pf, int func)
{
	char lo_cmd[12] = { ATAPI_LOCK, pf->lun << 5, 0, 0, func, 0, 0, 0, 0, 0, 0, 0 };

	pf_atapi(pf, lo_cmd, 0, pf_scratch, func ? "lock" : "unlock");
}

static void pf_eject(struct pf_unit *pf)
{
	char ej_cmd[12] = { ATAPI_DOOR, pf->lun << 5, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0 };

	pf_lock(pf, 0);
	pf_atapi(pf, ej_cmd, 0, pf_scratch, "eject");
}

#define PF_RESET_TMO   30	/* in tenths of a second */

static void pf_sleep(int cs)
{
	schedule_timeout_interruptible(cs);
}


static int pf_reset(struct pf_unit *pf)
{
	int i, k, flg;
	int expect[5] = { 1, 1, 1, 0x14, 0xeb };

	pi_connect(pf->pi);
	write_reg(pf, 6, 0xa0+0x10*pf->drive);
	write_reg(pf, 7, 8);

	pf_sleep(20 * HZ / 1000);

	k = 0;
	while ((k++ < PF_RESET_TMO) && (status_reg(pf) & STAT_BUSY))
		pf_sleep(HZ / 10);

	flg = 1;
	for (i = 0; i < 5; i++)
		flg &= (read_reg(pf, i + 1) == expect[i]);

	if (verbose) {
		printk("%s: Reset (%d) signature = ", pf->name, k);
		for (i = 0; i < 5; i++)
			printk("%3x", read_reg(pf, i + 1));
		if (!flg)
			printk(" (incorrect)");
		printk("\n");
	}

	pi_disconnect(pf->pi);
	return flg - 1;
}

static void pf_mode_sense(struct pf_unit *pf)
{
	char ms_cmd[12] =
	    { ATAPI_MODE_SENSE, pf->lun << 5, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0 };
	char buf[8];

	pf_atapi(pf, ms_cmd, 8, buf, "mode sense");
	pf->media_status = PF_RW;
	if (buf[3] & 0x80)
		pf->media_status = PF_RO;
}

static void xs(char *buf, char *targ, int offs, int len)
{
	int j, k, l;

	j = 0;
	l = 0;
	for (k = 0; k < len; k++)
		if ((buf[k + offs] != 0x20) || (buf[k + offs] != l))
			l = targ[j++] = buf[k + offs];
	if (l == 0x20)
		j--;
	targ[j] = 0;
}

static int xl(char *buf, int offs)
{
	int v, k;

	v = 0;
	for (k = 0; k < 4; k++)
		v = v * 256 + (buf[k + offs] & 0xff);
	return v;
}

static void pf_get_capacity(struct pf_unit *pf)
{
	char rc_cmd[12] = { ATAPI_CAPACITY, pf->lun << 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	char buf[8];
	int bs;

	if (pf_atapi(pf, rc_cmd, 8, buf, "get capacity")) {
		pf->media_status = PF_NM;
		return;
	}
	set_capacity(pf->disk, xl(buf, 0) + 1);
	bs = xl(buf, 4);
	if (bs != 512) {
		set_capacity(pf->disk, 0);
		if (verbose)
			printk("%s: Drive %d, LUN %d,"
			       " unsupported block size %d\n",
			       pf->name, pf->drive, pf->lun, bs);
	}
}

static int pf_identify(struct pf_unit *pf)
{
	int dt, s;
	char *ms[2] = { "master", "slave" };
	char mf[10], id[18];
	char id_cmd[12] =
	    { ATAPI_IDENTIFY, pf->lun << 5, 0, 0, 36, 0, 0, 0, 0, 0, 0, 0 };
	char buf[36];

	s = pf_atapi(pf, id_cmd, 36, buf, "identify");
	if (s)
		return -1;

	dt = buf[0] & 0x1f;
	if ((dt != 0) && (dt != 7)) {
		if (verbose)
			printk("%s: Drive %d, LUN %d, unsupported type %d\n",
			       pf->name, pf->drive, pf->lun, dt);
		return -1;
	}

	xs(buf, mf, 8, 8);
	xs(buf, id, 16, 16);

	pf->removable = (buf[1] & 0x80);

	pf_mode_sense(pf);
	pf_mode_sense(pf);
	pf_mode_sense(pf);

	pf_get_capacity(pf);

	printk("%s: %s %s, %s LUN %d, type %d",
	       pf->name, mf, id, ms[pf->drive], pf->lun, dt);
	if (pf->removable)
		printk(", removable");
	if (pf->media_status == PF_NM)
		printk(", no media\n");
	else {
		if (pf->media_status == PF_RO)
			printk(", RO");
		printk(", %llu blocks\n",
			(unsigned long long)get_capacity(pf->disk));
	}
	return 0;
}

static int pf_probe(struct pf_unit *pf)
{
	if (pf->drive == -1) {
		for (pf->drive = 0; pf->drive <= 1; pf->drive++)
			if (!pf_reset(pf)) {
				if (pf->lun != -1)
					return pf_identify(pf);
				else
					for (pf->lun = 0; pf->lun < 8; pf->lun++)
						if (!pf_identify(pf))
							return 0;
			}
	} else {
		if (pf_reset(pf))
			return -1;
		if (pf->lun != -1)
			return pf_identify(pf);
		for (pf->lun = 0; pf->lun < 8; pf->lun++)
			if (!pf_identify(pf))
				return 0;
	}
	return -1;
}

static int pf_detect(void)
{
	struct pf_unit *pf = units;
	int k, unit;

	printk("%s: %s version %s, major %d, cluster %d, nice %d\n",
	       name, name, PF_VERSION, major, cluster, nice);

	k = 0;
	if (pf_drive_count == 0) {
		if (pi_init(pf->pi, 1, -1, -1, -1, -1, -1, pf_scratch, PI_PF,
			    verbose, pf->name)) {
			if (!pf_probe(pf) && pf->disk) {
				pf->present = 1;
				k++;
			} else
				pi_release(pf->pi);
		}

	} else
		for (unit = 0; unit < PF_UNITS; unit++, pf++) {
			int *conf = *drives[unit];
			if (!conf[D_PRT])
				continue;
			if (pi_init(pf->pi, 0, conf[D_PRT], conf[D_MOD],
				    conf[D_UNI], conf[D_PRO], conf[D_DLY],
				    pf_scratch, PI_PF, verbose, pf->name)) {
				if (pf->disk && !pf_probe(pf)) {
					pf->present = 1;
					k++;
				} else
					pi_release(pf->pi);
			}
		}
	if (k)
		return 0;

	printk("%s: No ATAPI disk detected\n", name);
	for (pf = units, unit = 0; unit < PF_UNITS; pf++, unit++)
		put_disk(pf->disk);
	return -1;
}

/* The i/o request engine */

static int pf_start(struct pf_unit *pf, int cmd, int b, int c)
{
	int i;
	char io_cmd[12] = { cmd, pf->lun << 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	for (i = 0; i < 4; i++) {
		io_cmd[5 - i] = b & 0xff;
		b = b >> 8;
	}

	io_cmd[8] = c & 0xff;
	io_cmd[7] = (c >> 8) & 0xff;

	i = pf_command(pf, io_cmd, c * 512, "start i/o");

	mdelay(1);

	return i;
}

static int pf_ready(void)
{
	return (((status_reg(pf_current) & (STAT_BUSY | pf_mask)) == pf_mask));
}

static struct request_queue *pf_queue;

static void pf_end_request(int err)
{
	if (pf_req && !__blk_end_request_cur(pf_req, err))
		pf_req = NULL;
}

static void do_pf_request(struct request_queue * q)
{
	if (pf_busy)
		return;
repeat:
	if (!pf_req) {
		pf_req = blk_fetch_request(q);
		if (!pf_req)
			return;
	}

	pf_current = pf_req->rq_disk->private_data;
	pf_block = blk_rq_pos(pf_req);
	pf_run = blk_rq_sectors(pf_req);
	pf_count = blk_rq_cur_sectors(pf_req);

	if (pf_block + pf_count > get_capacity(pf_req->rq_disk)) {
		pf_end_request(-EIO);
		goto repeat;
	}

	pf_cmd = rq_data_dir(pf_req);
	pf_buf = pf_req->buffer;
	pf_retries = 0;

	pf_busy = 1;
	if (pf_cmd == READ)
		pi_do_claimed(pf_current->pi, do_pf_read);
	else if (pf_cmd == WRITE)
		pi_do_claimed(pf_current->pi, do_pf_write);
	else {
		pf_busy = 0;
		pf_end_request(-EIO);
		goto repeat;
	}
}

static int pf_next_buf(void)
{
	unsigned long saved_flags;

	pf_count--;
	pf_run--;
	pf_buf += 512;
	pf_block++;
	if (!pf_run)
		return 1;
	if (!pf_count) {
		spin_lock_irqsave(&pf_spin_lock, saved_flags);
		pf_end_request(0);
		spin_unlock_irqrestore(&pf_spin_lock, saved_flags);
		if (!pf_req)
			return 1;
		pf_count = blk_rq_cur_sectors(pf_req);
		pf_buf = pf_req->buffer;
	}
	return 0;
}

static inline void next_request(int err)
{
	unsigned long saved_flags;

	spin_lock_irqsave(&pf_spin_lock, saved_flags);
	pf_end_request(err);
	pf_busy = 0;
	do_pf_request(pf_queue);
	spin_unlock_irqrestore(&pf_spin_lock, saved_flags);
}

/* detach from the calling context - in case the spinlock is held */
static void do_pf_read(void)
{
	ps_set_intr(do_pf_read_start, NULL, 0, nice);
}

static void do_pf_read_start(void)
{
	pf_busy = 1;

	if (pf_start(pf_current, ATAPI_READ_10, pf_block, pf_run)) {
		pi_disconnect(pf_current->pi);
		if (pf_retries < PF_MAX_RETRIES) {
			pf_retries++;
			pi_do_claimed(pf_current->pi, do_pf_read_start);
			return;
		}
		next_request(-EIO);
		return;
	}
	pf_mask = STAT_DRQ;
	ps_set_intr(do_pf_read_drq, pf_ready, PF_TMO, nice);
}

static void do_pf_read_drq(void)
{
	while (1) {
		if (pf_wait(pf_current, STAT_BUSY, STAT_DRQ | STAT_ERR,
			    "read block", "completion") & STAT_ERR) {
			pi_disconnect(pf_current->pi);
			if (pf_retries < PF_MAX_RETRIES) {
				pf_req_sense(pf_current, 0);
				pf_retries++;
				pi_do_claimed(pf_current->pi, do_pf_read_start);
				return;
			}
			next_request(-EIO);
			return;
		}
		pi_read_block(pf_current->pi, pf_buf, 512);
		if (pf_next_buf())
			break;
	}
	pi_disconnect(pf_current->pi);
	next_request(0);
}

static void do_pf_write(void)
{
	ps_set_intr(do_pf_write_start, NULL, 0, nice);
}

static void do_pf_write_start(void)
{
	pf_busy = 1;

	if (pf_start(pf_current, ATAPI_WRITE_10, pf_block, pf_run)) {
		pi_disconnect(pf_current->pi);
		if (pf_retries < PF_MAX_RETRIES) {
			pf_retries++;
			pi_do_claimed(pf_current->pi, do_pf_write_start);
			return;
		}
		next_request(-EIO);
		return;
	}

	while (1) {
		if (pf_wait(pf_current, STAT_BUSY, STAT_DRQ | STAT_ERR,
			    "write block", "data wait") & STAT_ERR) {
			pi_disconnect(pf_current->pi);
			if (pf_retries < PF_MAX_RETRIES) {
				pf_retries++;
				pi_do_claimed(pf_current->pi, do_pf_write_start);
				return;
			}
			next_request(-EIO);
			return;
		}
		pi_write_block(pf_current->pi, pf_buf, 512);
		if (pf_next_buf())
			break;
	}
	pf_mask = 0;
	ps_set_intr(do_pf_write_done, pf_ready, PF_TMO, nice);
}

static void do_pf_write_done(void)
{
	if (pf_wait(pf_current, STAT_BUSY, 0, "write block", "done") & STAT_ERR) {
		pi_disconnect(pf_current->pi);
		if (pf_retries < PF_MAX_RETRIES) {
			pf_retries++;
			pi_do_claimed(pf_current->pi, do_pf_write_start);
			return;
		}
		next_request(-EIO);
		return;
	}
	pi_disconnect(pf_current->pi);
	next_request(0);
}

static int __init pf_init(void)
{				/* preliminary initialisation */
	struct pf_unit *pf;
	int unit;

	if (disable)
		return -EINVAL;

	pf_init_units();

	if (pf_detect())
		return -ENODEV;
	pf_busy = 0;

	if (register_blkdev(major, name)) {
		for (pf = units, unit = 0; unit < PF_UNITS; pf++, unit++)
			put_disk(pf->disk);
		return -EBUSY;
	}
	pf_queue = blk_init_queue(do_pf_request, &pf_spin_lock);
	if (!pf_queue) {
		unregister_blkdev(major, name);
		for (pf = units, unit = 0; unit < PF_UNITS; pf++, unit++)
			put_disk(pf->disk);
		return -ENOMEM;
	}

	blk_queue_max_segments(pf_queue, cluster);

	for (pf = units, unit = 0; unit < PF_UNITS; pf++, unit++) {
		struct gendisk *disk = pf->disk;

		if (!pf->present)
			continue;
		disk->private_data = pf;
		disk->queue = pf_queue;
		add_disk(disk);
	}
	return 0;
}

static void __exit pf_exit(void)
{
	struct pf_unit *pf;
	int unit;
	unregister_blkdev(major, name);
	for (pf = units, unit = 0; unit < PF_UNITS; pf++, unit++) {
		if (!pf->present)
			continue;
		del_gendisk(pf->disk);
		put_disk(pf->disk);
		pi_release(pf->pi);
	}
	blk_cleanup_queue(pf_queue);
}

MODULE_LICENSE("GPL");
module_init(pf_init)
module_exit(pf_exit)
