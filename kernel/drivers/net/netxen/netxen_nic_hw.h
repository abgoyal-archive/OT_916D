

#ifndef __NETXEN_NIC_HW_H_
#define __NETXEN_NIC_HW_H_

/* Hardware memory size of 128 meg */
#define NETXEN_MEMADDR_MAX (128 * 1024 * 1024)

struct netxen_adapter;

#define NETXEN_PCI_MAPSIZE_BYTES  (NETXEN_PCI_MAPSIZE << 20)

void netxen_nic_set_link_parameters(struct netxen_adapter *adapter);

/* Nibble or Byte mode for phy interface (GbE mode only) */

#define _netxen_crb_get_bit(var, bit)  ((var >> bit) & 0x1)


#define netxen_gb_tx_flowctl(config_word)	\
	((config_word) |= 1 << 4)
#define netxen_gb_rx_flowctl(config_word)	\
	((config_word) |= 1 << 5)
#define netxen_gb_tx_reset_pb(config_word)	\
	((config_word) |= 1 << 16)
#define netxen_gb_rx_reset_pb(config_word)	\
	((config_word) |= 1 << 17)
#define netxen_gb_tx_reset_mac(config_word)	\
	((config_word) |= 1 << 18)
#define netxen_gb_rx_reset_mac(config_word)	\
	((config_word) |= 1 << 19)

#define netxen_gb_unset_tx_flowctl(config_word)	\
	((config_word) &= ~(1 << 4))
#define netxen_gb_unset_rx_flowctl(config_word)	\
	((config_word) &= ~(1 << 5))

#define netxen_gb_get_tx_synced(config_word)	\
		_netxen_crb_get_bit((config_word), 1)
#define netxen_gb_get_rx_synced(config_word)	\
		_netxen_crb_get_bit((config_word), 3)
#define netxen_gb_get_tx_flowctl(config_word)	\
		_netxen_crb_get_bit((config_word), 4)
#define netxen_gb_get_rx_flowctl(config_word)	\
		_netxen_crb_get_bit((config_word), 5)
#define netxen_gb_get_soft_reset(config_word)	\
		_netxen_crb_get_bit((config_word), 31)

#define netxen_gb_get_stationaddress_low(config_word) ((config_word) >> 16)

#define netxen_gb_set_mii_mgmt_clockselect(config_word, val)	\
		((config_word) |= ((val) & 0x07))
#define netxen_gb_mii_mgmt_reset(config_word)	\
		((config_word) |= 1 << 31)
#define netxen_gb_mii_mgmt_unset(config_word)	\
		((config_word) &= ~(1 << 31))


#define netxen_gb_mii_mgmt_set_read_cycle(config_word)	\
		((config_word) |= 1 << 0)
#define netxen_gb_mii_mgmt_reg_addr(config_word, val)	\
		((config_word) |= ((val) & 0x1F))
#define netxen_gb_mii_mgmt_phy_addr(config_word, val)	\
		((config_word) |= (((val) & 0x1F) << 8))

#define netxen_get_gb_mii_mgmt_busy(config_word)	\
		_netxen_crb_get_bit(config_word, 0)
#define netxen_get_gb_mii_mgmt_scanning(config_word)	\
		_netxen_crb_get_bit(config_word, 1)
#define netxen_get_gb_mii_mgmt_notvalid(config_word)	\
		_netxen_crb_get_bit(config_word, 2)

#define netxen_xg_set_xg0_mask(config_word)    \
	((config_word) |= 1 << 0)
#define netxen_xg_set_xg1_mask(config_word)    \
	((config_word) |= 1 << 3)

#define netxen_xg_get_xg0_mask(config_word)    \
	_netxen_crb_get_bit((config_word), 0)
#define netxen_xg_get_xg1_mask(config_word)    \
	_netxen_crb_get_bit((config_word), 3)

#define netxen_xg_unset_xg0_mask(config_word)  \
	((config_word) &= ~(1 << 0))
#define netxen_xg_unset_xg1_mask(config_word)  \
	((config_word) &= ~(1 << 3))

#define netxen_gb_set_gb0_mask(config_word)    \
	((config_word) |= 1 << 0)
#define netxen_gb_set_gb1_mask(config_word)    \
	((config_word) |= 1 << 2)
#define netxen_gb_set_gb2_mask(config_word)    \
	((config_word) |= 1 << 4)
#define netxen_gb_set_gb3_mask(config_word)    \
	((config_word) |= 1 << 6)

#define netxen_gb_get_gb0_mask(config_word)    \
	_netxen_crb_get_bit((config_word), 0)
#define netxen_gb_get_gb1_mask(config_word)    \
	_netxen_crb_get_bit((config_word), 2)
#define netxen_gb_get_gb2_mask(config_word)    \
	_netxen_crb_get_bit((config_word), 4)
#define netxen_gb_get_gb3_mask(config_word)    \
	_netxen_crb_get_bit((config_word), 6)

#define netxen_gb_unset_gb0_mask(config_word)  \
	((config_word) &= ~(1 << 0))
#define netxen_gb_unset_gb1_mask(config_word)  \
	((config_word) &= ~(1 << 2))
#define netxen_gb_unset_gb2_mask(config_word)  \
	((config_word) &= ~(1 << 4))
#define netxen_gb_unset_gb3_mask(config_word)  \
	((config_word) &= ~(1 << 6))


#define NETXEN_NIU_GB_MII_MGMT_ADDR_CONTROL		0
#define NETXEN_NIU_GB_MII_MGMT_ADDR_STATUS		1
#define NETXEN_NIU_GB_MII_MGMT_ADDR_PHY_ID_0		2
#define NETXEN_NIU_GB_MII_MGMT_ADDR_PHY_ID_1		3
#define NETXEN_NIU_GB_MII_MGMT_ADDR_AUTONEG		4
#define NETXEN_NIU_GB_MII_MGMT_ADDR_LNKPART		5
#define NETXEN_NIU_GB_MII_MGMT_ADDR_AUTONEG_MORE	6
#define NETXEN_NIU_GB_MII_MGMT_ADDR_NEXTPAGE_XMIT	7
#define NETXEN_NIU_GB_MII_MGMT_ADDR_LNKPART_NEXTPAGE	8
#define NETXEN_NIU_GB_MII_MGMT_ADDR_1000BT_CONTROL	9
#define NETXEN_NIU_GB_MII_MGMT_ADDR_1000BT_STATUS	10
#define NETXEN_NIU_GB_MII_MGMT_ADDR_EXTENDED_STATUS	15
#define NETXEN_NIU_GB_MII_MGMT_ADDR_PHY_CONTROL		16
#define NETXEN_NIU_GB_MII_MGMT_ADDR_PHY_STATUS		17
#define NETXEN_NIU_GB_MII_MGMT_ADDR_INT_ENABLE		18
#define NETXEN_NIU_GB_MII_MGMT_ADDR_INT_STATUS		19
#define NETXEN_NIU_GB_MII_MGMT_ADDR_PHY_CONTROL_MORE	20
#define NETXEN_NIU_GB_MII_MGMT_ADDR_RECV_ERROR_COUNT	21
#define NETXEN_NIU_GB_MII_MGMT_ADDR_LED_CONTROL		24
#define NETXEN_NIU_GB_MII_MGMT_ADDR_LED_OVERRIDE	25
#define NETXEN_NIU_GB_MII_MGMT_ADDR_PHY_CONTROL_MORE_YET	26
#define NETXEN_NIU_GB_MII_MGMT_ADDR_PHY_STATUS_MORE	27


#define netxen_get_phy_speed(config_word) (((config_word) >> 14) & 0x03)

#define netxen_set_phy_speed(config_word, val)	\
		((config_word) |= ((val & 0x03) << 14))
#define netxen_set_phy_duplex(config_word)	\
		((config_word) |= 1 << 13)
#define netxen_clear_phy_duplex(config_word)	\
		((config_word) &= ~(1 << 13))

#define netxen_get_phy_link(config_word)	\
		_netxen_crb_get_bit(config_word, 10)
#define netxen_get_phy_duplex(config_word)	\
		_netxen_crb_get_bit(config_word, 13)


#define netxen_get_niu_enable_ge(config_word)	\
		_netxen_crb_get_bit(config_word, 1)

#define NETXEN_NIU_NON_PROMISC_MODE	0
#define NETXEN_NIU_PROMISC_MODE		1
#define NETXEN_NIU_ALLMULTI_MODE	2


#define netxen_xg_soft_reset(config_word)	\
		((config_word) |= 1 << 4)

typedef struct {
	unsigned valid;
	unsigned start_128M;
	unsigned end_128M;
	unsigned start_2M;
} crb_128M_2M_sub_block_map_t;

typedef struct {
	crb_128M_2M_sub_block_map_t sub_block[16];
} crb_128M_2M_block_map_t;

#endif				/* __NETXEN_NIC_HW_H_ */
