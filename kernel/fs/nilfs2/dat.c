

#include <linux/types.h>
#include <linux/buffer_head.h>
#include <linux/string.h>
#include <linux/errno.h>
#include "nilfs.h"
#include "mdt.h"
#include "alloc.h"
#include "dat.h"


#define NILFS_CNO_MIN	((__u64)1)
#define NILFS_CNO_MAX	(~(__u64)0)

struct nilfs_dat_info {
	struct nilfs_mdt_info mi;
	struct nilfs_palloc_cache palloc_cache;
};

static inline struct nilfs_dat_info *NILFS_DAT_I(struct inode *dat)
{
	return (struct nilfs_dat_info *)NILFS_MDT(dat);
}

static int nilfs_dat_prepare_entry(struct inode *dat,
				   struct nilfs_palloc_req *req, int create)
{
	return nilfs_palloc_get_entry_block(dat, req->pr_entry_nr,
					    create, &req->pr_entry_bh);
}

static void nilfs_dat_commit_entry(struct inode *dat,
				   struct nilfs_palloc_req *req)
{
	nilfs_mdt_mark_buffer_dirty(req->pr_entry_bh);
	nilfs_mdt_mark_dirty(dat);
	brelse(req->pr_entry_bh);
}

static void nilfs_dat_abort_entry(struct inode *dat,
				  struct nilfs_palloc_req *req)
{
	brelse(req->pr_entry_bh);
}

int nilfs_dat_prepare_alloc(struct inode *dat, struct nilfs_palloc_req *req)
{
	int ret;

	ret = nilfs_palloc_prepare_alloc_entry(dat, req);
	if (ret < 0)
		return ret;

	ret = nilfs_dat_prepare_entry(dat, req, 1);
	if (ret < 0)
		nilfs_palloc_abort_alloc_entry(dat, req);

	return ret;
}

void nilfs_dat_commit_alloc(struct inode *dat, struct nilfs_palloc_req *req)
{
	struct nilfs_dat_entry *entry;
	void *kaddr;

	kaddr = kmap_atomic(req->pr_entry_bh->b_page, KM_USER0);
	entry = nilfs_palloc_block_get_entry(dat, req->pr_entry_nr,
					     req->pr_entry_bh, kaddr);
	entry->de_start = cpu_to_le64(NILFS_CNO_MIN);
	entry->de_end = cpu_to_le64(NILFS_CNO_MAX);
	entry->de_blocknr = cpu_to_le64(0);
	kunmap_atomic(kaddr, KM_USER0);

	nilfs_palloc_commit_alloc_entry(dat, req);
	nilfs_dat_commit_entry(dat, req);
}

void nilfs_dat_abort_alloc(struct inode *dat, struct nilfs_palloc_req *req)
{
	nilfs_dat_abort_entry(dat, req);
	nilfs_palloc_abort_alloc_entry(dat, req);
}

void nilfs_dat_commit_free(struct inode *dat, struct nilfs_palloc_req *req)
{
	struct nilfs_dat_entry *entry;
	void *kaddr;

	kaddr = kmap_atomic(req->pr_entry_bh->b_page, KM_USER0);
	entry = nilfs_palloc_block_get_entry(dat, req->pr_entry_nr,
					     req->pr_entry_bh, kaddr);
	entry->de_start = cpu_to_le64(NILFS_CNO_MIN);
	entry->de_end = cpu_to_le64(NILFS_CNO_MIN);
	entry->de_blocknr = cpu_to_le64(0);
	kunmap_atomic(kaddr, KM_USER0);

	nilfs_dat_commit_entry(dat, req);
	nilfs_palloc_commit_free_entry(dat, req);
}

int nilfs_dat_prepare_start(struct inode *dat, struct nilfs_palloc_req *req)
{
	int ret;

	ret = nilfs_dat_prepare_entry(dat, req, 0);
	WARN_ON(ret == -ENOENT);
	return ret;
}

void nilfs_dat_commit_start(struct inode *dat, struct nilfs_palloc_req *req,
			    sector_t blocknr)
{
	struct nilfs_dat_entry *entry;
	void *kaddr;

	kaddr = kmap_atomic(req->pr_entry_bh->b_page, KM_USER0);
	entry = nilfs_palloc_block_get_entry(dat, req->pr_entry_nr,
					     req->pr_entry_bh, kaddr);
	entry->de_start = cpu_to_le64(nilfs_mdt_cno(dat));
	entry->de_blocknr = cpu_to_le64(blocknr);
	kunmap_atomic(kaddr, KM_USER0);

	nilfs_dat_commit_entry(dat, req);
}

int nilfs_dat_prepare_end(struct inode *dat, struct nilfs_palloc_req *req)
{
	struct nilfs_dat_entry *entry;
	__u64 start;
	sector_t blocknr;
	void *kaddr;
	int ret;

	ret = nilfs_dat_prepare_entry(dat, req, 0);
	if (ret < 0) {
		WARN_ON(ret == -ENOENT);
		return ret;
	}

	kaddr = kmap_atomic(req->pr_entry_bh->b_page, KM_USER0);
	entry = nilfs_palloc_block_get_entry(dat, req->pr_entry_nr,
					     req->pr_entry_bh, kaddr);
	start = le64_to_cpu(entry->de_start);
	blocknr = le64_to_cpu(entry->de_blocknr);
	kunmap_atomic(kaddr, KM_USER0);

	if (blocknr == 0) {
		ret = nilfs_palloc_prepare_free_entry(dat, req);
		if (ret < 0) {
			nilfs_dat_abort_entry(dat, req);
			return ret;
		}
	}

	return 0;
}

void nilfs_dat_commit_end(struct inode *dat, struct nilfs_palloc_req *req,
			  int dead)
{
	struct nilfs_dat_entry *entry;
	__u64 start, end;
	sector_t blocknr;
	void *kaddr;

	kaddr = kmap_atomic(req->pr_entry_bh->b_page, KM_USER0);
	entry = nilfs_palloc_block_get_entry(dat, req->pr_entry_nr,
					     req->pr_entry_bh, kaddr);
	end = start = le64_to_cpu(entry->de_start);
	if (!dead) {
		end = nilfs_mdt_cno(dat);
		WARN_ON(start > end);
	}
	entry->de_end = cpu_to_le64(end);
	blocknr = le64_to_cpu(entry->de_blocknr);
	kunmap_atomic(kaddr, KM_USER0);

	if (blocknr == 0)
		nilfs_dat_commit_free(dat, req);
	else
		nilfs_dat_commit_entry(dat, req);
}

void nilfs_dat_abort_end(struct inode *dat, struct nilfs_palloc_req *req)
{
	struct nilfs_dat_entry *entry;
	__u64 start;
	sector_t blocknr;
	void *kaddr;

	kaddr = kmap_atomic(req->pr_entry_bh->b_page, KM_USER0);
	entry = nilfs_palloc_block_get_entry(dat, req->pr_entry_nr,
					     req->pr_entry_bh, kaddr);
	start = le64_to_cpu(entry->de_start);
	blocknr = le64_to_cpu(entry->de_blocknr);
	kunmap_atomic(kaddr, KM_USER0);

	if (start == nilfs_mdt_cno(dat) && blocknr == 0)
		nilfs_palloc_abort_free_entry(dat, req);
	nilfs_dat_abort_entry(dat, req);
}

int nilfs_dat_prepare_update(struct inode *dat,
			     struct nilfs_palloc_req *oldreq,
			     struct nilfs_palloc_req *newreq)
{
	int ret;

	ret = nilfs_dat_prepare_end(dat, oldreq);
	if (!ret) {
		ret = nilfs_dat_prepare_alloc(dat, newreq);
		if (ret < 0)
			nilfs_dat_abort_end(dat, oldreq);
	}
	return ret;
}

void nilfs_dat_commit_update(struct inode *dat,
			     struct nilfs_palloc_req *oldreq,
			     struct nilfs_palloc_req *newreq, int dead)
{
	nilfs_dat_commit_end(dat, oldreq, dead);
	nilfs_dat_commit_alloc(dat, newreq);
}

void nilfs_dat_abort_update(struct inode *dat,
			    struct nilfs_palloc_req *oldreq,
			    struct nilfs_palloc_req *newreq)
{
	nilfs_dat_abort_end(dat, oldreq);
	nilfs_dat_abort_alloc(dat, newreq);
}

int nilfs_dat_mark_dirty(struct inode *dat, __u64 vblocknr)
{
	struct nilfs_palloc_req req;
	int ret;

	req.pr_entry_nr = vblocknr;
	ret = nilfs_dat_prepare_entry(dat, &req, 0);
	if (ret == 0)
		nilfs_dat_commit_entry(dat, &req);
	return ret;
}

int nilfs_dat_freev(struct inode *dat, __u64 *vblocknrs, size_t nitems)
{
	return nilfs_palloc_freev(dat, vblocknrs, nitems);
}

int nilfs_dat_move(struct inode *dat, __u64 vblocknr, sector_t blocknr)
{
	struct buffer_head *entry_bh;
	struct nilfs_dat_entry *entry;
	void *kaddr;
	int ret;

	ret = nilfs_palloc_get_entry_block(dat, vblocknr, 0, &entry_bh);
	if (ret < 0)
		return ret;
	kaddr = kmap_atomic(entry_bh->b_page, KM_USER0);
	entry = nilfs_palloc_block_get_entry(dat, vblocknr, entry_bh, kaddr);
	if (unlikely(entry->de_blocknr == cpu_to_le64(0))) {
		printk(KERN_CRIT "%s: vbn = %llu, [%llu, %llu)\n", __func__,
		       (unsigned long long)vblocknr,
		       (unsigned long long)le64_to_cpu(entry->de_start),
		       (unsigned long long)le64_to_cpu(entry->de_end));
		kunmap_atomic(kaddr, KM_USER0);
		brelse(entry_bh);
		return -EINVAL;
	}
	WARN_ON(blocknr == 0);
	entry->de_blocknr = cpu_to_le64(blocknr);
	kunmap_atomic(kaddr, KM_USER0);

	nilfs_mdt_mark_buffer_dirty(entry_bh);
	nilfs_mdt_mark_dirty(dat);

	brelse(entry_bh);

	return 0;
}

int nilfs_dat_translate(struct inode *dat, __u64 vblocknr, sector_t *blocknrp)
{
	struct buffer_head *entry_bh;
	struct nilfs_dat_entry *entry;
	sector_t blocknr;
	void *kaddr;
	int ret;

	ret = nilfs_palloc_get_entry_block(dat, vblocknr, 0, &entry_bh);
	if (ret < 0)
		return ret;

	kaddr = kmap_atomic(entry_bh->b_page, KM_USER0);
	entry = nilfs_palloc_block_get_entry(dat, vblocknr, entry_bh, kaddr);
	blocknr = le64_to_cpu(entry->de_blocknr);
	if (blocknr == 0) {
		ret = -ENOENT;
		goto out;
	}
	*blocknrp = blocknr;

 out:
	kunmap_atomic(kaddr, KM_USER0);
	brelse(entry_bh);
	return ret;
}

ssize_t nilfs_dat_get_vinfo(struct inode *dat, void *buf, unsigned visz,
			    size_t nvi)
{
	struct buffer_head *entry_bh;
	struct nilfs_dat_entry *entry;
	struct nilfs_vinfo *vinfo = buf;
	__u64 first, last;
	void *kaddr;
	unsigned long entries_per_block = NILFS_MDT(dat)->mi_entries_per_block;
	int i, j, n, ret;

	for (i = 0; i < nvi; i += n) {
		ret = nilfs_palloc_get_entry_block(dat, vinfo->vi_vblocknr,
						   0, &entry_bh);
		if (ret < 0)
			return ret;
		kaddr = kmap_atomic(entry_bh->b_page, KM_USER0);
		/* last virtual block number in this block */
		first = vinfo->vi_vblocknr;
		do_div(first, entries_per_block);
		first *= entries_per_block;
		last = first + entries_per_block - 1;
		for (j = i, n = 0;
		     j < nvi && vinfo->vi_vblocknr >= first &&
			     vinfo->vi_vblocknr <= last;
		     j++, n++, vinfo = (void *)vinfo + visz) {
			entry = nilfs_palloc_block_get_entry(
				dat, vinfo->vi_vblocknr, entry_bh, kaddr);
			vinfo->vi_start = le64_to_cpu(entry->de_start);
			vinfo->vi_end = le64_to_cpu(entry->de_end);
			vinfo->vi_blocknr = le64_to_cpu(entry->de_blocknr);
		}
		kunmap_atomic(kaddr, KM_USER0);
		brelse(entry_bh);
	}

	return nvi;
}

int nilfs_dat_read(struct inode *dat, struct nilfs_inode *raw_inode)
{
	return nilfs_read_inode_common(dat, raw_inode);
}

struct inode *nilfs_dat_new(struct the_nilfs *nilfs, size_t entry_size)
{
	static struct lock_class_key dat_lock_key;
	struct inode *dat;
	struct nilfs_dat_info *di;
	int err;

	dat = nilfs_mdt_new(nilfs, NULL, NILFS_DAT_INO, sizeof(*di));
	if (dat) {
		err = nilfs_palloc_init_blockgroup(dat, entry_size);
		if (unlikely(err)) {
			nilfs_mdt_destroy(dat);
			return NULL;
		}

		di = NILFS_DAT_I(dat);
		lockdep_set_class(&di->mi.mi_sem, &dat_lock_key);
		nilfs_palloc_setup_cache(dat, &di->palloc_cache);
	}
	return dat;
}
