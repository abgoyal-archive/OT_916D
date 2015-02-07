

#ifndef __NET_DCBNL_H__
#define __NET_DCBNL_H__

struct dcbnl_rtnl_ops {
	u8   (*getstate)(struct net_device *);
	u8   (*setstate)(struct net_device *, u8);
	void (*getpermhwaddr)(struct net_device *, u8 *);
	void (*setpgtccfgtx)(struct net_device *, int, u8, u8, u8, u8);
	void (*setpgbwgcfgtx)(struct net_device *, int, u8);
	void (*setpgtccfgrx)(struct net_device *, int, u8, u8, u8, u8);
	void (*setpgbwgcfgrx)(struct net_device *, int, u8);
	void (*getpgtccfgtx)(struct net_device *, int, u8 *, u8 *, u8 *, u8 *);
	void (*getpgbwgcfgtx)(struct net_device *, int, u8 *);
	void (*getpgtccfgrx)(struct net_device *, int, u8 *, u8 *, u8 *, u8 *);
	void (*getpgbwgcfgrx)(struct net_device *, int, u8 *);
	void (*setpfccfg)(struct net_device *, int, u8);
	void (*getpfccfg)(struct net_device *, int, u8 *);
	u8   (*setall)(struct net_device *);
	u8   (*getcap)(struct net_device *, int, u8 *);
	u8   (*getnumtcs)(struct net_device *, int, u8 *);
	u8   (*setnumtcs)(struct net_device *, int, u8);
	u8   (*getpfcstate)(struct net_device *);
	void (*setpfcstate)(struct net_device *, u8);
	void (*getbcncfg)(struct net_device *, int, u32 *);
	void (*setbcncfg)(struct net_device *, int, u32);
	void (*getbcnrp)(struct net_device *, int, u8 *);
	void (*setbcnrp)(struct net_device *, int, u8);
	u8   (*setapp)(struct net_device *, u8, u16, u8);
	u8   (*getapp)(struct net_device *, u8, u16);
};

#endif /* __NET_DCBNL_H__ */
