

#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/mlx4/qp.h>

#include "mlx4_en.h"

void mlx4_en_fill_qp_context(struct mlx4_en_priv *priv, int size, int stride,
			     int is_tx, int rss, int qpn, int cqn,
			     struct mlx4_qp_context *context)
{
	struct mlx4_en_dev *mdev = priv->mdev;

	memset(context, 0, sizeof *context);
	context->flags = cpu_to_be32(7 << 16 | rss << 13);
	context->pd = cpu_to_be32(mdev->priv_pdn);
	context->mtu_msgmax = 0xff;
	if (!is_tx && !rss)
		context->rq_size_stride = ilog2(size) << 3 | (ilog2(stride) - 4);
	if (is_tx)
		context->sq_size_stride = ilog2(size) << 3 | (ilog2(stride) - 4);
	else
		context->sq_size_stride = ilog2(TXBB_SIZE) - 4;
	context->usr_page = cpu_to_be32(mdev->priv_uar.index);
	context->local_qpn = cpu_to_be32(qpn);
	context->pri_path.ackto = 1 & 0x07;
	context->pri_path.sched_queue = 0x83 | (priv->port - 1) << 6;
	context->pri_path.counter_index = 0xff;
	context->cqn_send = cpu_to_be32(cqn);
	context->cqn_recv = cpu_to_be32(cqn);
	context->db_rec_addr = cpu_to_be64(priv->res.db.dma << 2);
}


int mlx4_en_map_buffer(struct mlx4_buf *buf)
{
	struct page **pages;
	int i;

	if (BITS_PER_LONG == 64 || buf->nbufs == 1)
		return 0;

	pages = kmalloc(sizeof *pages * buf->nbufs, GFP_KERNEL);
	if (!pages)
		return -ENOMEM;

	for (i = 0; i < buf->nbufs; ++i)
		pages[i] = virt_to_page(buf->page_list[i].buf);

	buf->direct.buf = vmap(pages, buf->nbufs, VM_MAP, PAGE_KERNEL);
	kfree(pages);
	if (!buf->direct.buf)
		return -ENOMEM;

	return 0;
}

void mlx4_en_unmap_buffer(struct mlx4_buf *buf)
{
	if (BITS_PER_LONG == 64 || buf->nbufs == 1)
		return;

	vunmap(buf->direct.buf);
}

void mlx4_en_sqp_event(struct mlx4_qp *qp, enum mlx4_event event)
{
    return;
}

