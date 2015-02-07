

#ifndef __WL_PCI_H__
#define __WL_PCI_H__




#define WL_LKM_PCI_VENDOR_ID    0x11C1  // Lucent Microelectronics
#define WL_LKM_PCI_DEVICE_ID_0  0xAB30  // Mini PCI
#define WL_LKM_PCI_DEVICE_ID_1  0xAB34  // Mini PCI
#define WL_LKM_PCI_DEVICE_ID_2  0xAB11  // WARP CardBus




int wl_adapter_init_module( void );

void wl_adapter_cleanup_module( void );

int wl_adapter_insert( struct net_device *dev );

int wl_adapter_open( struct net_device *dev );

int wl_adapter_close( struct net_device *dev );

int wl_adapter_is_open( struct net_device *dev );


#ifdef ENABLE_DMA

void wl_pci_dma_hcf_supply( struct wl_private *lp );

void wl_pci_dma_hcf_reclaim( struct wl_private *lp );

DESC_STRCT * wl_pci_dma_get_tx_packet( struct wl_private *lp );

void wl_pci_dma_put_tx_packet( struct wl_private *lp, DESC_STRCT *desc );

void wl_pci_dma_hcf_reclaim_tx( struct wl_private *lp );

#endif  // ENABLE_DMA


#endif  // __WL_PCI_H__
