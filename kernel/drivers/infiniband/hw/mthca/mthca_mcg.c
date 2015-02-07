

#include <linux/string.h>
#include <linux/gfp.h>

#include "mthca_dev.h"
#include "mthca_cmd.h"

struct mthca_mgm {
	__be32 next_gid_index;
	u32    reserved[3];
	u8     gid[16];
	__be32 qp[MTHCA_QP_PER_MGM];
};

static const u8 zero_gid[16];	/* automatically initialized to 0 */

static int find_mgm(struct mthca_dev *dev,
		    u8 *gid, struct mthca_mailbox *mgm_mailbox,
		    u16 *hash, int *prev, int *index)
{
	struct mthca_mailbox *mailbox;
	struct mthca_mgm *mgm = mgm_mailbox->buf;
	u8 *mgid;
	int err;
	u8 status;

	mailbox = mthca_alloc_mailbox(dev, GFP_KERNEL);
	if (IS_ERR(mailbox))
		return -ENOMEM;
	mgid = mailbox->buf;

	memcpy(mgid, gid, 16);

	err = mthca_MGID_HASH(dev, mailbox, hash, &status);
	if (err)
		goto out;
	if (status) {
		mthca_err(dev, "MGID_HASH returned status %02x\n", status);
		err = -EINVAL;
		goto out;
	}

	if (0)
		mthca_dbg(dev, "Hash for %pI6 is %04x\n", gid, *hash);

	*index = *hash;
	*prev  = -1;

	do {
		err = mthca_READ_MGM(dev, *index, mgm_mailbox, &status);
		if (err)
			goto out;
		if (status) {
			mthca_err(dev, "READ_MGM returned status %02x\n", status);
			err = -EINVAL;
			goto out;
		}

		if (!memcmp(mgm->gid, zero_gid, 16)) {
			if (*index != *hash) {
				mthca_err(dev, "Found zero MGID in AMGM.\n");
				err = -EINVAL;
			}
			goto out;
		}

		if (!memcmp(mgm->gid, gid, 16))
			goto out;

		*prev = *index;
		*index = be32_to_cpu(mgm->next_gid_index) >> 6;
	} while (*index);

	*index = -1;

 out:
	mthca_free_mailbox(dev, mailbox);
	return err;
}

int mthca_multicast_attach(struct ib_qp *ibqp, union ib_gid *gid, u16 lid)
{
	struct mthca_dev *dev = to_mdev(ibqp->device);
	struct mthca_mailbox *mailbox;
	struct mthca_mgm *mgm;
	u16 hash;
	int index, prev;
	int link = 0;
	int i;
	int err;
	u8 status;

	mailbox = mthca_alloc_mailbox(dev, GFP_KERNEL);
	if (IS_ERR(mailbox))
		return PTR_ERR(mailbox);
	mgm = mailbox->buf;

	mutex_lock(&dev->mcg_table.mutex);

	err = find_mgm(dev, gid->raw, mailbox, &hash, &prev, &index);
	if (err)
		goto out;

	if (index != -1) {
		if (!memcmp(mgm->gid, zero_gid, 16))
			memcpy(mgm->gid, gid->raw, 16);
	} else {
		link = 1;

		index = mthca_alloc(&dev->mcg_table.alloc);
		if (index == -1) {
			mthca_err(dev, "No AMGM entries left\n");
			err = -ENOMEM;
			goto out;
		}

		err = mthca_READ_MGM(dev, index, mailbox, &status);
		if (err)
			goto out;
		if (status) {
			mthca_err(dev, "READ_MGM returned status %02x\n", status);
			err = -EINVAL;
			goto out;
		}
		memset(mgm, 0, sizeof *mgm);
		memcpy(mgm->gid, gid->raw, 16);
	}

	for (i = 0; i < MTHCA_QP_PER_MGM; ++i)
		if (mgm->qp[i] == cpu_to_be32(ibqp->qp_num | (1 << 31))) {
			mthca_dbg(dev, "QP %06x already a member of MGM\n",
				  ibqp->qp_num);
			err = 0;
			goto out;
		} else if (!(mgm->qp[i] & cpu_to_be32(1 << 31))) {
			mgm->qp[i] = cpu_to_be32(ibqp->qp_num | (1 << 31));
			break;
		}

	if (i == MTHCA_QP_PER_MGM) {
		mthca_err(dev, "MGM at index %x is full.\n", index);
		err = -ENOMEM;
		goto out;
	}

	err = mthca_WRITE_MGM(dev, index, mailbox, &status);
	if (err)
		goto out;
	if (status) {
		mthca_err(dev, "WRITE_MGM returned status %02x\n", status);
		err = -EINVAL;
		goto out;
	}

	if (!link)
		goto out;

	err = mthca_READ_MGM(dev, prev, mailbox, &status);
	if (err)
		goto out;
	if (status) {
		mthca_err(dev, "READ_MGM returned status %02x\n", status);
		err = -EINVAL;
		goto out;
	}

	mgm->next_gid_index = cpu_to_be32(index << 6);

	err = mthca_WRITE_MGM(dev, prev, mailbox, &status);
	if (err)
		goto out;
	if (status) {
		mthca_err(dev, "WRITE_MGM returned status %02x\n", status);
		err = -EINVAL;
	}

 out:
	if (err && link && index != -1) {
		BUG_ON(index < dev->limits.num_mgms);
		mthca_free(&dev->mcg_table.alloc, index);
	}
	mutex_unlock(&dev->mcg_table.mutex);

	mthca_free_mailbox(dev, mailbox);
	return err;
}

int mthca_multicast_detach(struct ib_qp *ibqp, union ib_gid *gid, u16 lid)
{
	struct mthca_dev *dev = to_mdev(ibqp->device);
	struct mthca_mailbox *mailbox;
	struct mthca_mgm *mgm;
	u16 hash;
	int prev, index;
	int i, loc;
	int err;
	u8 status;

	mailbox = mthca_alloc_mailbox(dev, GFP_KERNEL);
	if (IS_ERR(mailbox))
		return PTR_ERR(mailbox);
	mgm = mailbox->buf;

	mutex_lock(&dev->mcg_table.mutex);

	err = find_mgm(dev, gid->raw, mailbox, &hash, &prev, &index);
	if (err)
		goto out;

	if (index == -1) {
		mthca_err(dev, "MGID %pI6 not found\n", gid->raw);
		err = -EINVAL;
		goto out;
	}

	for (loc = -1, i = 0; i < MTHCA_QP_PER_MGM; ++i) {
		if (mgm->qp[i] == cpu_to_be32(ibqp->qp_num | (1 << 31)))
			loc = i;
		if (!(mgm->qp[i] & cpu_to_be32(1 << 31)))
			break;
	}

	if (loc == -1) {
		mthca_err(dev, "QP %06x not found in MGM\n", ibqp->qp_num);
		err = -EINVAL;
		goto out;
	}

	mgm->qp[loc]   = mgm->qp[i - 1];
	mgm->qp[i - 1] = 0;

	err = mthca_WRITE_MGM(dev, index, mailbox, &status);
	if (err)
		goto out;
	if (status) {
		mthca_err(dev, "WRITE_MGM returned status %02x\n", status);
		err = -EINVAL;
		goto out;
	}

	if (i != 1)
		goto out;

	if (prev == -1) {
		/* Remove entry from MGM */
		int amgm_index_to_free = be32_to_cpu(mgm->next_gid_index) >> 6;
		if (amgm_index_to_free) {
			err = mthca_READ_MGM(dev, amgm_index_to_free,
					     mailbox, &status);
			if (err)
				goto out;
			if (status) {
				mthca_err(dev, "READ_MGM returned status %02x\n",
					  status);
				err = -EINVAL;
				goto out;
			}
		} else
			memset(mgm->gid, 0, 16);

		err = mthca_WRITE_MGM(dev, index, mailbox, &status);
		if (err)
			goto out;
		if (status) {
			mthca_err(dev, "WRITE_MGM returned status %02x\n", status);
			err = -EINVAL;
			goto out;
		}
		if (amgm_index_to_free) {
			BUG_ON(amgm_index_to_free < dev->limits.num_mgms);
			mthca_free(&dev->mcg_table.alloc, amgm_index_to_free);
		}
	} else {
		/* Remove entry from AMGM */
		int curr_next_index = be32_to_cpu(mgm->next_gid_index) >> 6;
		err = mthca_READ_MGM(dev, prev, mailbox, &status);
		if (err)
			goto out;
		if (status) {
			mthca_err(dev, "READ_MGM returned status %02x\n", status);
			err = -EINVAL;
			goto out;
		}

		mgm->next_gid_index = cpu_to_be32(curr_next_index << 6);

		err = mthca_WRITE_MGM(dev, prev, mailbox, &status);
		if (err)
			goto out;
		if (status) {
			mthca_err(dev, "WRITE_MGM returned status %02x\n", status);
			err = -EINVAL;
			goto out;
		}
		BUG_ON(index < dev->limits.num_mgms);
		mthca_free(&dev->mcg_table.alloc, index);
	}

 out:
	mutex_unlock(&dev->mcg_table.mutex);

	mthca_free_mailbox(dev, mailbox);
	return err;
}

int mthca_init_mcg_table(struct mthca_dev *dev)
{
	int err;
	int table_size = dev->limits.num_mgms + dev->limits.num_amgms;

	err = mthca_alloc_init(&dev->mcg_table.alloc,
			       table_size,
			       table_size - 1,
			       dev->limits.num_mgms);
	if (err)
		return err;

	mutex_init(&dev->mcg_table.mutex);

	return 0;
}

void mthca_cleanup_mcg_table(struct mthca_dev *dev)
{
	mthca_alloc_cleanup(&dev->mcg_table.alloc);
}
