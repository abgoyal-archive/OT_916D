
#ifndef __LINUX_DS1286_H
#define __LINUX_DS1286_H

#define RTC_HUNDREDTH_SECOND	0
#define RTC_SECONDS		1
#define RTC_MINUTES		2
#define RTC_MINUTES_ALARM	3
#define RTC_HOURS		4
#define RTC_HOURS_ALARM		5
#define RTC_DAY			6
#define RTC_DAY_ALARM		7
#define RTC_DATE		8
#define RTC_MONTH		9
#define RTC_YEAR		10
#define RTC_CMD			11
#define RTC_WHSEC		12
#define RTC_WSEC		13
#define RTC_UNUSED		14

/* RTC_*_alarm is always true if 2 MSBs are set */
# define RTC_ALARM_DONT_CARE 	0xC0


#define RTC_EOSC		0x80
#define RTC_ESQW		0x40

#define RTC_TDF			0x01
#define RTC_WAF			0x02
#define RTC_TDM			0x04
#define RTC_WAM			0x08
#define RTC_PU_LVL		0x10
#define RTC_IBH_LO		0x20
#define RTC_IPSW		0x40
#define RTC_TE			0x80

#endif /* __LINUX_DS1286_H */
