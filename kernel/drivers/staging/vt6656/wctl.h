

#ifndef __WCTL_H__
#define __WCTL_H__

#include "ttype.h"
#include "tether.h"
#include "device.h"

/*---------------------  Export Definitions -------------------------*/

#define IS_TYPE_DATA(pMACHeader)                                                        \
    ((((PS802_11Header) pMACHeader)->wFrameCtl & TYPE_802_11_MASK) == TYPE_802_11_DATA)

#define IS_TYPE_MGMT(pMACHeader)                                                        \
    ((((PS802_11Header) pMACHeader)->wFrameCtl & TYPE_802_11_MASK) == TYPE_802_11_MGMT)

#define IS_TYPE_CONTROL(pMACHeader)                                                     \
    ((((PS802_11Header) pMACHeader)->wFrameCtl & TYPE_802_11_MASK) == TYPE_802_11_CTL)

#define IS_FC_MOREDATA(pMACHeader)                                                      \
    ((((PS802_11Header) pMACHeader)->wFrameCtl & FC_MOREDATA) == FC_MOREDATA)

#define IS_FC_POWERMGT(pMACHeader)                                                      \
    ((((PS802_11Header) pMACHeader)->wFrameCtl & FC_POWERMGT) == FC_POWERMGT)

#define IS_FC_RETRY(pMACHeader)                                                         \
    ((((PS802_11Header) pMACHeader)->wFrameCtl & FC_RETRY) == FC_RETRY)

#define IS_FC_WEP(pMACHeader)                                                           \
    ((((PS802_11Header) pMACHeader)->wFrameCtl & FC_WEP) == FC_WEP)

#ifdef __BIG_ENDIAN

#define IS_FRAGMENT_PKT(pMACHeader)                                                     \
    (((((PS802_11Header) pMACHeader)->wFrameCtl & FC_MOREFRAG) != 0) |                  \
     ((((PS802_11Header) pMACHeader)->wSeqCtl & 0x0F00) != 0))

#define IS_FIRST_FRAGMENT_PKT(pMACHeader)                                               \
    ((((PS802_11Header) pMACHeader)->wSeqCtl & 0x0F00) == 0)

#else

#define IS_FRAGMENT_PKT(pMACHeader)                                                     \
    (((((PS802_11Header) pMACHeader)->wFrameCtl & FC_MOREFRAG) != 0) |                  \
     ((((PS802_11Header) pMACHeader)->wSeqCtl & 0x000F) != 0))

#define IS_FIRST_FRAGMENT_PKT(pMACHeader)                                               \
    ((((PS802_11Header) pMACHeader)->wSeqCtl & 0x000F) == 0)

#endif//#ifdef __BIG_ENDIAN

#define IS_LAST_FRAGMENT_PKT(pMACHeader)                                                \
    ((((PS802_11Header) pMACHeader)->wFrameCtl & FC_MOREFRAG) == 0)

#define IS_CTL_PSPOLL(pMACHeader)                                                       \
    ((((PS802_11Header) pMACHeader)->wFrameCtl & TYPE_SUBTYPE_MASK) == TYPE_CTL_PSPOLL)


#define ADD_ONE_WITH_WRAP_AROUND(uVar, uModulo) {   \
    if ((uVar) >= ((uModulo) - 1))                  \
        (uVar) = 0;                                 \
    else                                            \
        (uVar)++;                                   \
}

/*---------------------  Export Classes  ----------------------------*/

/*---------------------  Export Variables  --------------------------*/

/*---------------------  Export Functions  --------------------------*/

BOOL WCTLbIsDuplicate(PSCache pCache, PS802_11Header pMACHeader);
BOOL WCTLbHandleFragment(PSDevice pDevice, PS802_11Header pMACHeader,
			 unsigned int cbFrameLength, BOOL bWEP, BOOL bExtIV);
unsigned int WCTLuSearchDFCB(PSDevice pDevice, PS802_11Header pMACHeader);
unsigned int WCTLuInsertDFCB(PSDevice pDevice, PS802_11Header pMACHeader);

#endif /* __WCTL_H__ */
