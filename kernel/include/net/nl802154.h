

#ifndef IEEE802154_NL_H
#define IEEE802154_NL_H

struct net_device;
struct ieee802154_addr;

int ieee802154_nl_assoc_indic(struct net_device *dev,
		struct ieee802154_addr *addr, u8 cap);

int ieee802154_nl_assoc_confirm(struct net_device *dev,
		u16 short_addr, u8 status);

int ieee802154_nl_disassoc_indic(struct net_device *dev,
		struct ieee802154_addr *addr, u8 reason);

int ieee802154_nl_disassoc_confirm(struct net_device *dev,
		u8 status);

int ieee802154_nl_scan_confirm(struct net_device *dev,
		u8 status, u8 scan_type, u32 unscanned, u8 page,
		u8 *edl/*, struct list_head *pan_desc_list */);

int ieee802154_nl_beacon_indic(struct net_device *dev, u16 panid,
		u16 coord_addr);

int ieee802154_nl_start_confirm(struct net_device *dev, u8 status);

#endif
