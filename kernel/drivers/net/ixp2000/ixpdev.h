

#ifndef __IXPDEV_H
#define __IXPDEV_H

struct ixpdev_priv
{
	struct net_device *dev;
	struct napi_struct napi;
	int	channel;
	int	tx_queue_entries;
};

struct net_device *ixpdev_alloc(int channel, int sizeof_priv);
int ixpdev_init(int num_ports, struct net_device **nds,
		void (*set_port_admin_status)(int port, int up));
void ixpdev_deinit(void);


#endif
