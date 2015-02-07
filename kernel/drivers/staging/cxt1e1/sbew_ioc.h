

#ifndef _INC_SBEWIOC_H_
#define _INC_SBEWIOC_H_


#ifndef __KERNEL__
#include <sys/types.h>
#endif
#ifdef SunOS
#include <sys/ioccom.h>
#else
#include <linux/ioctl.h>
#endif

#ifdef __cplusplus
extern      "C"
{
#endif

#define SBE_LOCKFILE   "/tmp/.sbewan.LCK"

#define SBE_IOC_COOKIE     0x19780926
#define SBE_IOC_MAGIC      ('s')

/* IOW write - data has to go into driver from application */
/* IOR read - data has to be returned to application from driver */


#define SBE_IOC_LOGLEVEL       _IOW(SBE_IOC_MAGIC, 0x00, int)
#define SBE_IOC_CHAN_NEW       _IOW(SBE_IOC_MAGIC, 0x01,int)    /* unused */
#define SBE_IOC_CHAN_UP        _IOW(SBE_IOC_MAGIC, 0x02,int)    /* unused */
#define SBE_IOC_CHAN_DOWN      _IOW(SBE_IOC_MAGIC, 0x03,int)    /* unused */
#define SBE_IOC_CHAN_GET       _IOWR(SBE_IOC_MAGIC,0x04, struct sbecom_chan_param)
#define SBE_IOC_CHAN_SET       _IOW(SBE_IOC_MAGIC, 0x05, struct sbecom_chan_param)
#define SBE_IOC_CHAN_GET_STAT  _IOWR(SBE_IOC_MAGIC,0x06, struct sbecom_chan_stats)
#define SBE_IOC_CHAN_DEL_STAT  _IOW(SBE_IOC_MAGIC, 0x07, int)
#define SBE_IOC_PORTS_ENABLE   _IOW(SBE_IOC_MAGIC, 0x0A, int)
#define SBE_IOC_PORT_GET       _IOWR(SBE_IOC_MAGIC,0x0C, struct sbecom_port_param)
#define SBE_IOC_PORT_SET       _IOW(SBE_IOC_MAGIC, 0x0D, struct sbecom_port_param)
#define SBE_IOC_READ_VEC       _IOWR(SBE_IOC_MAGIC,0x10, struct sbecom_wrt_vec)
#define SBE_IOC_WRITE_VEC      _IOWR(SBE_IOC_MAGIC,0x11, struct sbecom_wrt_vec)
#define SBE_IOC_GET_SN         _IOR(SBE_IOC_MAGIC, 0x12, u_int32_t)
#define SBE_IOC_RESET_DEV      _IOW(SBE_IOC_MAGIC, 0x13, int)
#define SBE_IOC_FRAMER_GET     _IOWR(SBE_IOC_MAGIC,0x14, struct sbecom_framer_param)
#define SBE_IOC_FRAMER_SET     _IOW(SBE_IOC_MAGIC, 0x15, struct sbecom_framer_param)
#define SBE_IOC_CARD_GET       _IOR(SBE_IOC_MAGIC, 0x20, struct sbecom_card_param)
#define SBE_IOC_CARD_SET       _IOW(SBE_IOC_MAGIC, 0x21, struct sbecom_card_param)
#define SBE_IOC_CARD_GET_STAT  _IOR(SBE_IOC_MAGIC, 0x22, struct temux_card_stats)
#define SBE_IOC_CARD_DEL_STAT  _IO(SBE_IOC_MAGIC,  0x23)
#define SBE_IOC_CARD_CHAN_STAT _IOR(SBE_IOC_MAGIC, 0x24, struct sbecom_chan_stats)
#define SBE_IOC_CARD_BLINK     _IOW(SBE_IOC_MAGIC, 0x30, int)
#define SBE_IOC_DRVINFO_GET    _IOWR(SBE_IOC_MAGIC,0x31, struct sbe_drv_info)
#define SBE_IOC_BRDINFO_GET    _IOR(SBE_IOC_MAGIC, 0x32, struct sbe_brd_info)
#define SBE_IOC_IID_GET        _IOWR(SBE_IOC_MAGIC,0x33, struct sbe_iid_info)
#define SBE_IOC_BRDADDR_GET    _IOWR(SBE_IOC_MAGIC, 0x34, struct sbe_brd_addr)

#ifdef NOT_YET_COMMON
#define SBE_IOC_TSIOC_GET      _IOWR(SBE_IOC_MAGIC,0x16, struct wanc1t3_ts_param)
#define SBE_IOC_TSIOC_SET      _IOW(SBE_IOC_MAGIC, 0x17, struct wanc1t3_ts_param)
#endif


#define SBE_IOC_MAXVEC    1


#ifdef __cplusplus
}
#endif

#endif                          /*** _INC_SBEWIOC_H_ ***/
