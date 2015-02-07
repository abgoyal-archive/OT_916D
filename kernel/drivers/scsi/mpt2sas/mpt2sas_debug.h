

#ifndef MPT2SAS_DEBUG_H_INCLUDED
#define MPT2SAS_DEBUG_H_INCLUDED

#define MPT_DEBUG			0x00000001
#define MPT_DEBUG_MSG_FRAME		0x00000002
#define MPT_DEBUG_SG			0x00000004
#define MPT_DEBUG_EVENTS		0x00000008
#define MPT_DEBUG_EVENT_WORK_TASK	0x00000010
#define MPT_DEBUG_INIT			0x00000020
#define MPT_DEBUG_EXIT			0x00000040
#define MPT_DEBUG_FAIL			0x00000080
#define MPT_DEBUG_TM			0x00000100
#define MPT_DEBUG_REPLY			0x00000200
#define MPT_DEBUG_HANDSHAKE		0x00000400
#define MPT_DEBUG_CONFIG		0x00000800
#define MPT_DEBUG_DL			0x00001000
#define MPT_DEBUG_RESET			0x00002000
#define MPT_DEBUG_SCSI			0x00004000
#define MPT_DEBUG_IOCTL			0x00008000
#define MPT_DEBUG_CSMISAS		0x00010000
#define MPT_DEBUG_SAS			0x00020000
#define MPT_DEBUG_TRANSPORT		0x00040000
#define MPT_DEBUG_TASK_SET_FULL		0x00080000

#define MPT_DEBUG_TARGET_MODE		0x00100000



#ifdef CONFIG_SCSI_MPT2SAS_LOGGING
#define MPT_CHECK_LOGGING(IOC, CMD, BITS)			\
{								\
	if (IOC->logging_level & BITS)				\
		CMD;						\
}
#else
#define MPT_CHECK_LOGGING(IOC, CMD, BITS)
#endif /* CONFIG_SCSI_MPT2SAS_LOGGING */



#define dprintk(IOC, CMD)			\
	MPT_CHECK_LOGGING(IOC, CMD, MPT_DEBUG)

#define dsgprintk(IOC, CMD)			\
	MPT_CHECK_LOGGING(IOC, CMD, MPT_DEBUG_SG)

#define devtprintk(IOC, CMD)			\
	MPT_CHECK_LOGGING(IOC, CMD, MPT_DEBUG_EVENTS)

#define dewtprintk(IOC, CMD)		\
	MPT_CHECK_LOGGING(IOC, CMD, MPT_DEBUG_EVENT_WORK_TASK)

#define dinitprintk(IOC, CMD)			\
	MPT_CHECK_LOGGING(IOC, CMD, MPT_DEBUG_INIT)

#define dexitprintk(IOC, CMD)			\
	MPT_CHECK_LOGGING(IOC, CMD, MPT_DEBUG_EXIT)

#define dfailprintk(IOC, CMD)			\
	MPT_CHECK_LOGGING(IOC, CMD, MPT_DEBUG_FAIL)

#define dtmprintk(IOC, CMD)			\
	MPT_CHECK_LOGGING(IOC, CMD, MPT_DEBUG_TM)

#define dreplyprintk(IOC, CMD)			\
	MPT_CHECK_LOGGING(IOC, CMD, MPT_DEBUG_REPLY)

#define dhsprintk(IOC, CMD)			\
	MPT_CHECK_LOGGING(IOC, CMD, MPT_DEBUG_HANDSHAKE)

#define dcprintk(IOC, CMD)			\
	MPT_CHECK_LOGGING(IOC, CMD, MPT_DEBUG_CONFIG)

#define ddlprintk(IOC, CMD)			\
	MPT_CHECK_LOGGING(IOC, CMD, MPT_DEBUG_DL)

#define drsprintk(IOC, CMD)			\
	MPT_CHECK_LOGGING(IOC, CMD, MPT_DEBUG_RESET)

#define dsprintk(IOC, CMD)			\
	MPT_CHECK_LOGGING(IOC, CMD, MPT_DEBUG_SCSI)

#define dctlprintk(IOC, CMD)			\
	MPT_CHECK_LOGGING(IOC, CMD, MPT_DEBUG_IOCTL)

#define dcsmisasprintk(IOC, CMD)		\
	MPT_CHECK_LOGGING(IOC, CMD, MPT_DEBUG_CSMISAS)

#define dsasprintk(IOC, CMD)			\
	MPT_CHECK_LOGGING(IOC, CMD, MPT_DEBUG_SAS)

#define dsastransport(IOC, CMD)		\
	MPT_CHECK_LOGGING(IOC, CMD, MPT_DEBUG_SAS_WIDE)

#define dmfprintk(IOC, CMD)			\
	MPT_CHECK_LOGGING(IOC, CMD, MPT_DEBUG_MSG_FRAME)

#define dtsfprintk(IOC, CMD)			\
	MPT_CHECK_LOGGING(IOC, CMD, MPT_DEBUG_TASK_SET_FULL)

#define dtransportprintk(IOC, CMD)			\
	MPT_CHECK_LOGGING(IOC, CMD, MPT_DEBUG_TRANSPORT)

#define dTMprintk(IOC, CMD)			\
	MPT_CHECK_LOGGING(IOC, CMD, MPT_DEBUG_TARGET_MODE)

/* inline functions for dumping debug data*/
#ifdef CONFIG_SCSI_MPT2SAS_LOGGING
static inline void
_debug_dump_mf(void *mpi_request, int sz)
{
	int i;
	u32 *mfp = (u32 *)mpi_request;

	printk(KERN_INFO "mf:\n\t");
	for (i = 0; i < sz; i++) {
		if (i && ((i % 8) == 0))
			printk("\n\t");
		printk("%08x ", le32_to_cpu(mfp[i]));
	}
	printk("\n");
}
#else
#define _debug_dump_mf(mpi_request, sz)
#endif /* CONFIG_SCSI_MPT2SAS_LOGGING */

#endif /* MPT2SAS_DEBUG_H_INCLUDED */
