

#include <linux/slab.h>

#include "mlx4.h"

struct mlx4_device_context {
	struct list_head	list;
	struct mlx4_interface  *intf;
	void		       *context;
};

static LIST_HEAD(intf_list);
static LIST_HEAD(dev_list);
static DEFINE_MUTEX(intf_mutex);

static void mlx4_add_device(struct mlx4_interface *intf, struct mlx4_priv *priv)
{
	struct mlx4_device_context *dev_ctx;

	dev_ctx = kmalloc(sizeof *dev_ctx, GFP_KERNEL);
	if (!dev_ctx)
		return;

	dev_ctx->intf    = intf;
	dev_ctx->context = intf->add(&priv->dev);

	if (dev_ctx->context) {
		spin_lock_irq(&priv->ctx_lock);
		list_add_tail(&dev_ctx->list, &priv->ctx_list);
		spin_unlock_irq(&priv->ctx_lock);
	} else
		kfree(dev_ctx);
}

static void mlx4_remove_device(struct mlx4_interface *intf, struct mlx4_priv *priv)
{
	struct mlx4_device_context *dev_ctx;

	list_for_each_entry(dev_ctx, &priv->ctx_list, list)
		if (dev_ctx->intf == intf) {
			spin_lock_irq(&priv->ctx_lock);
			list_del(&dev_ctx->list);
			spin_unlock_irq(&priv->ctx_lock);

			intf->remove(&priv->dev, dev_ctx->context);
			kfree(dev_ctx);
			return;
		}
}

int mlx4_register_interface(struct mlx4_interface *intf)
{
	struct mlx4_priv *priv;

	if (!intf->add || !intf->remove)
		return -EINVAL;

	mutex_lock(&intf_mutex);

	list_add_tail(&intf->list, &intf_list);
	list_for_each_entry(priv, &dev_list, dev_list)
		mlx4_add_device(intf, priv);

	mutex_unlock(&intf_mutex);

	return 0;
}
EXPORT_SYMBOL_GPL(mlx4_register_interface);

void mlx4_unregister_interface(struct mlx4_interface *intf)
{
	struct mlx4_priv *priv;

	mutex_lock(&intf_mutex);

	list_for_each_entry(priv, &dev_list, dev_list)
		mlx4_remove_device(intf, priv);

	list_del(&intf->list);

	mutex_unlock(&intf_mutex);
}
EXPORT_SYMBOL_GPL(mlx4_unregister_interface);

void mlx4_dispatch_event(struct mlx4_dev *dev, enum mlx4_dev_event type, int port)
{
	struct mlx4_priv *priv = mlx4_priv(dev);
	struct mlx4_device_context *dev_ctx;
	unsigned long flags;

	spin_lock_irqsave(&priv->ctx_lock, flags);

	list_for_each_entry(dev_ctx, &priv->ctx_list, list)
		if (dev_ctx->intf->event)
			dev_ctx->intf->event(dev, dev_ctx->context, type, port);

	spin_unlock_irqrestore(&priv->ctx_lock, flags);
}

int mlx4_register_device(struct mlx4_dev *dev)
{
	struct mlx4_priv *priv = mlx4_priv(dev);
	struct mlx4_interface *intf;

	mutex_lock(&intf_mutex);

	list_add_tail(&priv->dev_list, &dev_list);
	list_for_each_entry(intf, &intf_list, list)
		mlx4_add_device(intf, priv);

	mutex_unlock(&intf_mutex);
	mlx4_start_catas_poll(dev);

	return 0;
}

void mlx4_unregister_device(struct mlx4_dev *dev)
{
	struct mlx4_priv *priv = mlx4_priv(dev);
	struct mlx4_interface *intf;

	mlx4_stop_catas_poll(dev);
	mutex_lock(&intf_mutex);

	list_for_each_entry(intf, &intf_list, list)
		mlx4_remove_device(intf, priv);

	list_del(&priv->dev_list);

	mutex_unlock(&intf_mutex);
}
