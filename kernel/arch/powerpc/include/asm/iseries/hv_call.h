
#ifndef _ASM_POWERPC_ISERIES_HV_CALL_H
#define _ASM_POWERPC_ISERIES_HV_CALL_H

#include <asm/iseries/hv_call_sc.h>
#include <asm/iseries/hv_types.h>
#include <asm/paca.h>

/* Type of yield for HvCallBaseYieldProcessor */
#define HvCall_YieldTimed	0	/* Yield until specified time (tb) */
#define HvCall_YieldToActive	1	/* Yield until all active procs have run */
#define HvCall_YieldToProc	2	/* Yield until the specified processor has run */

/* interrupt masks for setEnabledInterrupts */
#define HvCall_MaskIPI		0x00000001
#define HvCall_MaskLpEvent	0x00000002
#define HvCall_MaskLpProd	0x00000004
#define HvCall_MaskTimeout	0x00000008

/* Log buffer formats */
#define HvCall_LogBuffer_ASCII          0
#define HvCall_LogBuffer_EBCDIC         1

#define HvCallBaseAckDeferredInts			HvCallBase +  0
#define HvCallBaseCpmPowerOff				HvCallBase +  1
#define HvCallBaseGetHwPatch				HvCallBase +  2
#define HvCallBaseReIplSpAttn				HvCallBase +  3
#define HvCallBaseSetASR				HvCallBase +  4
#define HvCallBaseSetASRAndRfi				HvCallBase +  5
#define HvCallBaseSetIMR				HvCallBase +  6
#define HvCallBaseSendIPI				HvCallBase +  7
#define HvCallBaseTerminateMachine			HvCallBase +  8
#define HvCallBaseTerminateMachineSrc			HvCallBase +  9
#define HvCallBaseProcessPlicInterrupts			HvCallBase + 10
#define HvCallBaseIsPrimaryCpmOrMsdIpl			HvCallBase + 11
#define HvCallBaseSetVirtualSIT				HvCallBase + 12
#define HvCallBaseVaryOffThisProcessor			HvCallBase + 13
#define HvCallBaseVaryOffMemoryChunk			HvCallBase + 14
#define HvCallBaseVaryOffInteractivePercentage		HvCallBase + 15
#define HvCallBaseSendLpProd				HvCallBase + 16
#define HvCallBaseSetEnabledInterrupts			HvCallBase + 17
#define HvCallBaseYieldProcessor			HvCallBase + 18
#define HvCallBaseVaryOffSharedProcUnits		HvCallBase + 19
#define HvCallBaseSetVirtualDecr			HvCallBase + 20
#define HvCallBaseClearLogBuffer			HvCallBase + 21
#define HvCallBaseGetLogBufferCodePage			HvCallBase + 22
#define HvCallBaseGetLogBufferFormat			HvCallBase + 23
#define HvCallBaseGetLogBufferLength			HvCallBase + 24
#define HvCallBaseReadLogBuffer				HvCallBase + 25
#define HvCallBaseSetLogBufferFormatAndCodePage		HvCallBase + 26
#define HvCallBaseWriteLogBuffer			HvCallBase + 27
#define HvCallBaseRouter28				HvCallBase + 28
#define HvCallBaseRouter29				HvCallBase + 29
#define HvCallBaseRouter30				HvCallBase + 30
#define HvCallBaseSetDebugBus				HvCallBase + 31

#define HvCallCcSetDABR					HvCallCc + 7

static inline void HvCall_setVirtualDecr(void)
{
	/*
	 * Ignore any error return codes - most likely means that the
	 * target value for the LP has been increased and this vary off
	 * would bring us below the new target.
	 */
	HvCall0(HvCallBaseSetVirtualDecr);
}

static inline void HvCall_yieldProcessor(unsigned typeOfYield, u64 yieldParm)
{
	HvCall2(HvCallBaseYieldProcessor, typeOfYield, yieldParm);
}

static inline void HvCall_setEnabledInterrupts(u64 enabledInterrupts)
{
	HvCall1(HvCallBaseSetEnabledInterrupts, enabledInterrupts);
}

static inline void HvCall_setLogBufferFormatAndCodepage(int format,
		u32 codePage)
{
	HvCall2(HvCallBaseSetLogBufferFormatAndCodePage, format, codePage);
}

extern void HvCall_writeLogBuffer(const void *buffer, u64 bufLen);

static inline void HvCall_sendIPI(struct paca_struct *targetPaca)
{
	HvCall1(HvCallBaseSendIPI, targetPaca->paca_index);
}

#endif /* _ASM_POWERPC_ISERIES_HV_CALL_H */
