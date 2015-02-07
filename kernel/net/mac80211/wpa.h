

#ifndef WPA_H
#define WPA_H

#include <linux/skbuff.h>
#include <linux/types.h>
#include "ieee80211_i.h"

ieee80211_tx_result
ieee80211_tx_h_michael_mic_add(struct ieee80211_tx_data *tx);
ieee80211_rx_result
ieee80211_rx_h_michael_mic_verify(struct ieee80211_rx_data *rx);

ieee80211_tx_result
ieee80211_crypto_tkip_encrypt(struct ieee80211_tx_data *tx);
ieee80211_rx_result
ieee80211_crypto_tkip_decrypt(struct ieee80211_rx_data *rx);

ieee80211_tx_result
ieee80211_crypto_ccmp_encrypt(struct ieee80211_tx_data *tx);
ieee80211_rx_result
ieee80211_crypto_ccmp_decrypt(struct ieee80211_rx_data *rx);

ieee80211_tx_result
ieee80211_crypto_aes_cmac_encrypt(struct ieee80211_tx_data *tx);
ieee80211_rx_result
ieee80211_crypto_aes_cmac_decrypt(struct ieee80211_rx_data *rx);

#endif /* WPA_H */
