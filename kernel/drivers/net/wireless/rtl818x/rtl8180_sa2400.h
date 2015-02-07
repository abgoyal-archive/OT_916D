
#ifndef RTL8180_SA2400_H
#define RTL8180_SA2400_H


#define SA2400_ANTENNA 0x91
#define SA2400_DIG_ANAPARAM_PWR1_ON 0x8
#define SA2400_ANA_ANAPARAM_PWR1_ON 0x28
#define SA2400_ANAPARAM_PWR0_ON 0x3

/* RX sensitivity in dbm */
#define SA2400_MAX_SENS 85

#define SA2400_REG4_FIRDAC_SHIFT 7

extern const struct rtl818x_rf_ops sa2400_rf_ops;

#endif /* RTL8180_SA2400_H */
