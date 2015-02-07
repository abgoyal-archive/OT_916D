

#include <linux/workqueue.h>

#include "mlx4.h"

enum {
	MLX4_CATAS_POLL_INTERVAL	= 5 * HZ,
};

static DEFINE_SPINLOCK(catas_lock);

static LIST_HEAD(catas_list);
static struct work_struct catas_work;

static int internal_err_reset = 1;
module_param(internal_err_reset, int, 0644);
MODULE_PARM_DESC(internal_err_reset,
		 "Reset device on internal errors if non-zero (default 1)");

static void dump_err_buf(struct mlx4_dev *dev)
{
	struct mlx4_priv *priv = mlx4_priv(dev);

	int i;

	mlx4_err(dev, "Internal error detected:\n");
	for (i = 0; i < priv->fw.catas_size; ++i)
		mlx4_err(dev, "  buf[%02x]: %08x\n",
			 i, swab32(readl(priv->catas_err.map + i)));
}

static void poll_catas(unsigned long dev_ptr)
{
	struct mlx4_dev *dev = (struct mlx4_dev *) dev_ptr;
	struct mlx4_priv *priv = mlx4_priv(dev);

	if (readl(priv->catas_err.map)) {
		dump_err_buf(dev);

		mlx4_dispatch_event(dev, MLX4_DEV_EVENT_CATASTROPHIC_ERROR, 0);

		if (internal_err_reset) {
			spin_lock(&catas_lock);
			list_add(&priv->catas_err.list, &catas_list);
			spin_unlock(&catas_lock);

			queue_work(mlx4_wq, &catas_work);
		}
	} else
		mod_timer(&priv->catas_err.timer,
			  round_jiffies(jiffies + MLX4_CATAS_POLL_INTERVAL));
}

static void catas_reset(struct work_struct *work)
{
	struct mlx4_priv *priv, *tmppriv;
	struct mlx4_dev *dev;

	LIST_HEAD(tlist);
	int ret;

	spin_lock_irq(&catas_lock);
	list_splice_init(&catas_list, &tlist);
	spin_unlock_irq(&catas_lock);

	list_for_each_entry_safe(priv, tmppriv, &tlist, catas_err.list) {
		struct pci_dev *pdev = priv->dev.pdev;

		ret = mlx4_restart_one(priv->dev.pdev);
		/* 'priv' now is not valid */
		if (ret)
			printk(KERN_ERR "mlx4 %s: Reset failed (%d)\n",
				pci_name(pdev), ret);
		else {
			dev  = pci_get_drvdata(pdev);
			mlx4_dbg(dev, "Reset succeeded\n");
		}
	}
}

void mlx4_start_catas_poll(struct mlx4_dev *dev)
{
	struct mlx4_priv *priv = mlx4_priv(dev);
	unsigned long addr;

	INIT_LIST_HEAD(&priv->catas_err.list);
	init_timer(&priv->catas_err.timer);
	priv->catas_err.map = NULL;

	addr = pci_resource_start(dev->pdev, priv->fw.catas_bar) +
		priv->fw.catas_offset;

	priv->catas_err.map = ioremap(addr, priv->fw.catas_size * 4);
	if (!priv->catas_err.map) {
		mlx4_warn(dev, "Failed to map internal error buffer at 0x%lx\n",
			  addr);
		return;
	}

	priv->catas_err.timer.data     = (unsigned long) dev;
	priv->catas_err.timer.function = poll_catas;
	priv->catas_err.timer.expires  =
		round_jiffies(jiffies + MLX4_CATAS_POLL_INTERVAL);
	add_timer(&priv->catas_err.timer);
}

void mlx4_stop_catas_poll(struct mlx4_dev *dev)
{
	struct mlx4_priv *priv = mlx4_priv(dev);

	del_timer_sync(&priv->catas_err.timer);

	if (priv->catas_err.map)
		iounmap(priv->catas_err.map);

	spin_lock_irq(&catas_lock);
	list_del(&priv->catas_err.list);
	spin_unlock_irq(&catas_lock);
}

void  __init mlx4_catas_init(void)
{
	INIT_WORK(&catas_work, catas_reset);
}
