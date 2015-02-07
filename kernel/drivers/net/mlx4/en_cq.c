

#include <linux/mlx4/cq.h>
#include <linux/mlx4/qp.h>
#include <linux/mlx4/cmd.h>

#include "mlx4_en.h"

static void mlx4_en_cq_event(struct mlx4_cq *cq, enum mlx4_event event)
{
	return;
}


int mlx4_en_create_cq(struct mlx4_en_priv *priv,
		      struct mlx4_en_cq *cq,
		      int entries, int ring, enum cq_type mode)
{
	struct mlx4_en_dev *mdev = priv->mdev;
	int err;

	cq->size = entries;
	if (mode == RX) {
		cq->buf_size = cq->size * sizeof(struct mlx4_cqe);
		cq->vector   = ring % mdev->dev->caps.num_comp_vectors;
	} else {
		cq->buf_size = sizeof(struct mlx4_cqe);
		cq->vector   = 0;
	}

	cq->ring = ring;
	cq->is_tx = mode;
	spin_lock_init(&cq->lock);

	err = mlx4_alloc_hwq_res(mdev->dev, &cq->wqres,
				cq->buf_size, 2 * PAGE_SIZE);
	if (err)
		return err;

	err = mlx4_en_map_buffer(&cq->wqres.buf);
	if (err)
		mlx4_free_hwq_res(mdev->dev, &cq->wqres, cq->buf_size);
	else
		cq->buf = (struct mlx4_cqe *) cq->wqres.buf.direct.buf;

	return err;
}

int mlx4_en_activate_cq(struct mlx4_en_priv *priv, struct mlx4_en_cq *cq)
{
	struct mlx4_en_dev *mdev = priv->mdev;
	int err;

	cq->dev = mdev->pndev[priv->port];
	cq->mcq.set_ci_db  = cq->wqres.db.db;
	cq->mcq.arm_db     = cq->wqres.db.db + 1;
	*cq->mcq.set_ci_db = 0;
	*cq->mcq.arm_db    = 0;
	memset(cq->buf, 0, cq->buf_size);

	if (!cq->is_tx)
		cq->size = priv->rx_ring[cq->ring].actual_size;

	err = mlx4_cq_alloc(mdev->dev, cq->size, &cq->wqres.mtt, &mdev->priv_uar,
			    cq->wqres.db.dma, &cq->mcq, cq->vector, cq->is_tx);
	if (err)
		return err;

	cq->mcq.comp  = cq->is_tx ? mlx4_en_tx_irq : mlx4_en_rx_irq;
	cq->mcq.event = mlx4_en_cq_event;

	if (cq->is_tx) {
		init_timer(&cq->timer);
		cq->timer.function = mlx4_en_poll_tx_cq;
		cq->timer.data = (unsigned long) cq;
	} else {
		netif_napi_add(cq->dev, &cq->napi, mlx4_en_poll_rx_cq, 64);
		napi_enable(&cq->napi);
	}

	return 0;
}

void mlx4_en_destroy_cq(struct mlx4_en_priv *priv, struct mlx4_en_cq *cq)
{
	struct mlx4_en_dev *mdev = priv->mdev;

	mlx4_en_unmap_buffer(&cq->wqres.buf);
	mlx4_free_hwq_res(mdev->dev, &cq->wqres, cq->buf_size);
	cq->buf_size = 0;
	cq->buf = NULL;
}

void mlx4_en_deactivate_cq(struct mlx4_en_priv *priv, struct mlx4_en_cq *cq)
{
	struct mlx4_en_dev *mdev = priv->mdev;

	if (cq->is_tx)
		del_timer(&cq->timer);
	else {
		napi_disable(&cq->napi);
		netif_napi_del(&cq->napi);
	}

	mlx4_cq_free(mdev->dev, &cq->mcq);
}

/* Set rx cq moderation parameters */
int mlx4_en_set_cq_moder(struct mlx4_en_priv *priv, struct mlx4_en_cq *cq)
{
	return mlx4_cq_modify(priv->mdev->dev, &cq->mcq,
			      cq->moder_cnt, cq->moder_time);
}

int mlx4_en_arm_cq(struct mlx4_en_priv *priv, struct mlx4_en_cq *cq)
{
	mlx4_cq_arm(&cq->mcq, MLX4_CQ_DB_REQ_NOT, priv->mdev->uar_map,
		    &priv->mdev->uar_lock);

	return 0;
}


