

#include "tmacro.h"
#include "tether.h"
#include "80211mgr.h"
#include "80211hdr.h"
#include "device.h"
#include "wpa.h"

/*---------------------  Static Definitions -------------------------*/



/*---------------------  Static Classes  ----------------------------*/

/*---------------------  Static Variables  --------------------------*/

static int          msglevel                =MSG_LEVEL_INFO;
//static int          msglevel                =MSG_LEVEL_DEBUG;
/*---------------------  Static Functions  --------------------------*/



/*---------------------  Export Variables  --------------------------*/


/*---------------------  Export Functions  --------------------------*/



void
vMgrEncodeBeacon(
    PWLAN_FR_BEACON  pFrame
     )
{
    pFrame->pHdr = (PUWLAN_80211HDR)pFrame->pBuf;

    // Fixed Fields
    pFrame->pqwTimestamp = (PQWORD)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3))
                                    + WLAN_BEACON_OFF_TS);
    pFrame->pwBeaconInterval = (PWORD)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3))
                                       + WLAN_BEACON_OFF_BCN_INT);
    pFrame->pwCapInfo = (PWORD)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3))
                                + WLAN_BEACON_OFF_CAPINFO);

    pFrame->len = WLAN_HDR_ADDR3_LEN + WLAN_BEACON_OFF_SSID;

    return;
}



void
vMgrDecodeBeacon(
    PWLAN_FR_BEACON  pFrame
    )
{
    PWLAN_IE        pItem;

    pFrame->pHdr = (PUWLAN_80211HDR)pFrame->pBuf;

    // Fixed Fields
    pFrame->pqwTimestamp = (PQWORD)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3))
                                    + WLAN_BEACON_OFF_TS);
    pFrame->pwBeaconInterval = (PWORD)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3))
                                       + WLAN_BEACON_OFF_BCN_INT);
    pFrame->pwCapInfo = (PWORD)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3))
                                + WLAN_BEACON_OFF_CAPINFO);

    // Information elements
    pItem = (PWLAN_IE)((PBYTE)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3)))
                       + WLAN_BEACON_OFF_SSID);
    while( ((PBYTE)pItem) < (pFrame->pBuf + pFrame->len) ){

        switch (pItem->byElementID) {
            case WLAN_EID_SSID:
                if (pFrame->pSSID == NULL)
                    pFrame->pSSID = (PWLAN_IE_SSID)pItem;
                break;
            case WLAN_EID_SUPP_RATES:
                if (pFrame->pSuppRates == NULL)
                    pFrame->pSuppRates = (PWLAN_IE_SUPP_RATES)pItem;
                break;
            case WLAN_EID_FH_PARMS:
                //pFrame->pFHParms = (PWLAN_IE_FH_PARMS)pItem;
                break;
            case WLAN_EID_DS_PARMS:
                if (pFrame->pDSParms == NULL)
                    pFrame->pDSParms = (PWLAN_IE_DS_PARMS)pItem;
                break;
            case WLAN_EID_CF_PARMS:
                if (pFrame->pCFParms == NULL)
                    pFrame->pCFParms = (PWLAN_IE_CF_PARMS)pItem;
                break;
            case WLAN_EID_IBSS_PARMS:
                if (pFrame->pIBSSParms == NULL)
                    pFrame->pIBSSParms = (PWLAN_IE_IBSS_PARMS)pItem;
                break;
            case WLAN_EID_TIM:
                if (pFrame->pTIM == NULL)
                    pFrame->pTIM = (PWLAN_IE_TIM)pItem;
                break;

            case WLAN_EID_RSN:
                if (pFrame->pRSN == NULL) {
                    pFrame->pRSN = (PWLAN_IE_RSN)pItem;
                }
                break;
            case WLAN_EID_RSN_WPA:
                if (pFrame->pRSNWPA == NULL) {
                    if (WPAb_Is_RSN((PWLAN_IE_RSN_EXT)pItem) == TRUE)
                        pFrame->pRSNWPA = (PWLAN_IE_RSN_EXT)pItem;
                }
                break;

            case WLAN_EID_ERP:
                if (pFrame->pERP == NULL)
                    pFrame->pERP = (PWLAN_IE_ERP)pItem;
                break;
            case WLAN_EID_EXTSUPP_RATES:
                if (pFrame->pExtSuppRates == NULL)
                    pFrame->pExtSuppRates = (PWLAN_IE_SUPP_RATES)pItem;
                break;

            case WLAN_EID_COUNTRY:      //7
                if (pFrame->pIE_Country == NULL)
                    pFrame->pIE_Country = (PWLAN_IE_COUNTRY)pItem;
                break;

            case WLAN_EID_PWR_CONSTRAINT:   //32
                if (pFrame->pIE_PowerConstraint == NULL)
                    pFrame->pIE_PowerConstraint = (PWLAN_IE_PW_CONST)pItem;
                break;

            case WLAN_EID_CH_SWITCH:    //37
                if (pFrame->pIE_CHSW == NULL)
                    pFrame->pIE_CHSW = (PWLAN_IE_CH_SW)pItem;
                break;

            case WLAN_EID_QUIET:        //40
                if (pFrame->pIE_Quiet == NULL)
                    pFrame->pIE_Quiet = (PWLAN_IE_QUIET)pItem;
                break;

            case WLAN_EID_IBSS_DFS:
                if (pFrame->pIE_IBSSDFS == NULL)
                    pFrame->pIE_IBSSDFS = (PWLAN_IE_IBSS_DFS)pItem;
                break;

            default:
                DBG_PRT(MSG_LEVEL_DEBUG, KERN_INFO "Unrecognized EID=%dd in beacon decode.\n", pItem->byElementID);
                break;

        }
        pItem = (PWLAN_IE)(((PBYTE)pItem) + 2 + pItem->len);
    }

    return;
}




void
vMgrEncodeIBSSATIM(
    PWLAN_FR_IBSSATIM   pFrame
    )
{
    pFrame->pHdr = (PUWLAN_80211HDR)pFrame->pBuf;
    pFrame->len = WLAN_HDR_ADDR3_LEN;

    return;
}



void
vMgrDecodeIBSSATIM(
    PWLAN_FR_IBSSATIM   pFrame
    )
{
    pFrame->pHdr = (PUWLAN_80211HDR)pFrame->pBuf;

    return;
}



void
vMgrEncodeDisassociation(
    PWLAN_FR_DISASSOC  pFrame
    )
{
    pFrame->pHdr = (PUWLAN_80211HDR)pFrame->pBuf;


    // Fixed Fields
    pFrame->pwReason = (PWORD)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3))
                               + WLAN_DISASSOC_OFF_REASON);
    pFrame->len = WLAN_HDR_ADDR3_LEN + WLAN_DISASSOC_OFF_REASON + sizeof(*(pFrame->pwReason));

    return;
}



void
vMgrDecodeDisassociation(
    PWLAN_FR_DISASSOC  pFrame
    )
{
    pFrame->pHdr = (PUWLAN_80211HDR)pFrame->pBuf;

    // Fixed Fields
    pFrame->pwReason = (PWORD)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3))
                               + WLAN_DISASSOC_OFF_REASON);

    return;
}



void
vMgrEncodeAssocRequest(
    PWLAN_FR_ASSOCREQ  pFrame
    )
{
    pFrame->pHdr = (PUWLAN_80211HDR)pFrame->pBuf;
    // Fixed Fields
    pFrame->pwCapInfo = (PWORD)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3))
                                + WLAN_ASSOCREQ_OFF_CAP_INFO);
    pFrame->pwListenInterval = (PWORD)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3))
                                       + WLAN_ASSOCREQ_OFF_LISTEN_INT);
    pFrame->len = WLAN_HDR_ADDR3_LEN + WLAN_ASSOCREQ_OFF_LISTEN_INT + sizeof(*(pFrame->pwListenInterval));
    return;
}



void
vMgrDecodeAssocRequest(
    PWLAN_FR_ASSOCREQ  pFrame
    )
{
    PWLAN_IE   pItem;

    pFrame->pHdr = (PUWLAN_80211HDR)pFrame->pBuf;
    // Fixed Fields
    pFrame->pwCapInfo = (PWORD)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3))
                                + WLAN_ASSOCREQ_OFF_CAP_INFO);
    pFrame->pwListenInterval = (PWORD)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3))
                                + WLAN_ASSOCREQ_OFF_LISTEN_INT);

    // Information elements
    pItem = (PWLAN_IE)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3))
                            + WLAN_ASSOCREQ_OFF_SSID);

    while (((PBYTE)pItem) < (pFrame->pBuf + pFrame->len)) {
        switch (pItem->byElementID){
            case WLAN_EID_SSID:
                if (pFrame->pSSID == NULL)
                    pFrame->pSSID = (PWLAN_IE_SSID)pItem;
                break;
            case WLAN_EID_SUPP_RATES:
                if (pFrame->pSuppRates == NULL)
                    pFrame->pSuppRates = (PWLAN_IE_SUPP_RATES)pItem;
                break;

            case WLAN_EID_RSN:
                if (pFrame->pRSN == NULL) {
                    pFrame->pRSN = (PWLAN_IE_RSN)pItem;
                }
                break;
            case WLAN_EID_RSN_WPA:
                if (pFrame->pRSNWPA == NULL) {
                    if (WPAb_Is_RSN((PWLAN_IE_RSN_EXT)pItem) == TRUE)
                        pFrame->pRSNWPA = (PWLAN_IE_RSN_EXT)pItem;
                }
                break;
            case WLAN_EID_EXTSUPP_RATES:
                if (pFrame->pExtSuppRates == NULL)
                    pFrame->pExtSuppRates = (PWLAN_IE_SUPP_RATES)pItem;
                break;

            default:
                DBG_PRT(MSG_LEVEL_DEBUG, KERN_INFO "Unrecognized EID=%dd in assocreq decode.\n",
                        pItem->byElementID);
                break;
        }
        pItem = (PWLAN_IE)(((PBYTE)pItem) + 2 + pItem->len);
    }
    return;
}


void
vMgrEncodeAssocResponse(
    PWLAN_FR_ASSOCRESP  pFrame
     )
{
    pFrame->pHdr = (PUWLAN_80211HDR)pFrame->pBuf;

    // Fixed Fields
    pFrame->pwCapInfo = (PWORD)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3))
                                + WLAN_ASSOCRESP_OFF_CAP_INFO);
    pFrame->pwStatus = (PWORD)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3))
                               + WLAN_ASSOCRESP_OFF_STATUS);
    pFrame->pwAid = (PWORD)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3))
                            + WLAN_ASSOCRESP_OFF_AID);
    pFrame->len = WLAN_HDR_ADDR3_LEN + WLAN_ASSOCRESP_OFF_AID
                  + sizeof(*(pFrame->pwAid));

    return;
}



void
vMgrDecodeAssocResponse(
    PWLAN_FR_ASSOCRESP  pFrame
     )
{
    PWLAN_IE   pItem;

    pFrame->pHdr = (PUWLAN_80211HDR)pFrame->pBuf;

    // Fixed Fields
    pFrame->pwCapInfo = (PWORD)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3))
                                + WLAN_ASSOCRESP_OFF_CAP_INFO);
    pFrame->pwStatus = (PWORD)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3))
                               + WLAN_ASSOCRESP_OFF_STATUS);
    pFrame->pwAid = (PWORD)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3))
                            + WLAN_ASSOCRESP_OFF_AID);

    // Information elements
    pFrame->pSuppRates  = (PWLAN_IE_SUPP_RATES)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3))
                           + WLAN_ASSOCRESP_OFF_SUPP_RATES);

    pItem = (PWLAN_IE)(pFrame->pSuppRates);
    pItem = (PWLAN_IE)(((PBYTE)pItem) + 2 + pItem->len);

    if ((((PBYTE)pItem) < (pFrame->pBuf + pFrame->len)) && (pItem->byElementID == WLAN_EID_EXTSUPP_RATES)) {
        pFrame->pExtSuppRates = (PWLAN_IE_SUPP_RATES)pItem;
        DBG_PRT(MSG_LEVEL_DEBUG, KERN_INFO "pFrame->pExtSuppRates=[%p].\n", pItem);
    }
    else {
        pFrame->pExtSuppRates = NULL;
    }
    return;
}



void
vMgrEncodeReassocRequest(
    PWLAN_FR_REASSOCREQ  pFrame
     )
{
    pFrame->pHdr = (PUWLAN_80211HDR)pFrame->pBuf;

    // Fixed Fields
    pFrame->pwCapInfo = (PWORD)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3))
                                + WLAN_REASSOCREQ_OFF_CAP_INFO);
    pFrame->pwListenInterval = (PWORD)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3))
                                      + WLAN_REASSOCREQ_OFF_LISTEN_INT);
    pFrame->pAddrCurrAP = (PIEEE_ADDR)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3))
                                       + WLAN_REASSOCREQ_OFF_CURR_AP);
    pFrame->len = WLAN_HDR_ADDR3_LEN + WLAN_REASSOCREQ_OFF_CURR_AP + sizeof(*(pFrame->pAddrCurrAP));

    return;
}




void
vMgrDecodeReassocRequest(
    PWLAN_FR_REASSOCREQ  pFrame
     )
{
    PWLAN_IE   pItem;
    pFrame->pHdr = (PUWLAN_80211HDR)pFrame->pBuf;

    // Fixed Fields
    pFrame->pwCapInfo = (PWORD)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3))
                                + WLAN_REASSOCREQ_OFF_CAP_INFO);
    pFrame->pwListenInterval = (PWORD)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3))
                                      + WLAN_REASSOCREQ_OFF_LISTEN_INT);
    pFrame->pAddrCurrAP = (PIEEE_ADDR)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3))
                                       + WLAN_REASSOCREQ_OFF_CURR_AP);

    // Information elements
    pItem = (PWLAN_IE)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3))
                       + WLAN_REASSOCREQ_OFF_SSID);

    while(((PBYTE)pItem) < (pFrame->pBuf + pFrame->len)) {

        switch (pItem->byElementID){
            case WLAN_EID_SSID:
                if (pFrame->pSSID == NULL)
                    pFrame->pSSID = (PWLAN_IE_SSID)pItem;
                break;
            case WLAN_EID_SUPP_RATES:
                if (pFrame->pSuppRates == NULL)
                    pFrame->pSuppRates = (PWLAN_IE_SUPP_RATES)pItem;
                break;

            case WLAN_EID_RSN:
                if (pFrame->pRSN == NULL) {
                    pFrame->pRSN = (PWLAN_IE_RSN)pItem;
                }
                break;
            case WLAN_EID_RSN_WPA:
                if (pFrame->pRSNWPA == NULL) {
                    if (WPAb_Is_RSN((PWLAN_IE_RSN_EXT)pItem) == TRUE)
                        pFrame->pRSNWPA = (PWLAN_IE_RSN_EXT)pItem;
                }
                break;

            case WLAN_EID_EXTSUPP_RATES:
                if (pFrame->pExtSuppRates == NULL)
                    pFrame->pExtSuppRates = (PWLAN_IE_SUPP_RATES)pItem;
                break;
            default:
                DBG_PRT(MSG_LEVEL_DEBUG, KERN_INFO "Unrecognized EID=%dd in reassocreq decode.\n",
                            pItem->byElementID);
                break;
        }
        pItem = (PWLAN_IE)(((PBYTE)pItem) + 2 + pItem->len);
    }
    return;
}





void
vMgrEncodeProbeRequest(
    PWLAN_FR_PROBEREQ  pFrame
     )
{
    pFrame->pHdr = (PUWLAN_80211HDR)pFrame->pBuf;
    pFrame->len = WLAN_HDR_ADDR3_LEN;
    return;
}


void
vMgrDecodeProbeRequest(
    PWLAN_FR_PROBEREQ  pFrame
     )
{
    PWLAN_IE   pItem;

    pFrame->pHdr = (PUWLAN_80211HDR)pFrame->pBuf;

    // Information elements
    pItem = (PWLAN_IE)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3)));

    while( ((PBYTE)pItem) < (pFrame->pBuf + pFrame->len) ) {

        switch (pItem->byElementID) {
            case WLAN_EID_SSID:
                if (pFrame->pSSID == NULL)
                    pFrame->pSSID = (PWLAN_IE_SSID)pItem;
                break;

            case WLAN_EID_SUPP_RATES:
                if (pFrame->pSuppRates == NULL)
                    pFrame->pSuppRates = (PWLAN_IE_SUPP_RATES)pItem;
                break;

            case WLAN_EID_EXTSUPP_RATES:
                if (pFrame->pExtSuppRates == NULL)
                    pFrame->pExtSuppRates = (PWLAN_IE_SUPP_RATES)pItem;
                break;

            default:
                DBG_PRT(MSG_LEVEL_DEBUG, KERN_INFO "Bad EID=%dd in probereq\n", pItem->byElementID);
                break;
        }

        pItem = (PWLAN_IE)(((PBYTE)pItem) + 2 +  pItem->len);
    }
    return;
}




void
vMgrEncodeProbeResponse(
    PWLAN_FR_PROBERESP  pFrame
    )
{
    pFrame->pHdr = (PUWLAN_80211HDR)pFrame->pBuf;

    // Fixed Fields
    pFrame->pqwTimestamp = (PQWORD)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3))
                                    + WLAN_PROBERESP_OFF_TS);
    pFrame->pwBeaconInterval = (PWORD)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3))
                                       + WLAN_PROBERESP_OFF_BCN_INT);
    pFrame->pwCapInfo = (PWORD)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3))
                                + WLAN_PROBERESP_OFF_CAP_INFO);

    pFrame->len = WLAN_HDR_ADDR3_LEN + WLAN_PROBERESP_OFF_CAP_INFO +
                  sizeof(*(pFrame->pwCapInfo));

    return;
}




void
vMgrDecodeProbeResponse(
    PWLAN_FR_PROBERESP  pFrame
    )
{
    PWLAN_IE    pItem;


    pFrame->pHdr = (PUWLAN_80211HDR)pFrame->pBuf;

    // Fixed Fields
    pFrame->pqwTimestamp = (PQWORD)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3))
                                    + WLAN_PROBERESP_OFF_TS);
    pFrame->pwBeaconInterval = (PWORD)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3))
                                       + WLAN_PROBERESP_OFF_BCN_INT);
    pFrame->pwCapInfo = (PWORD)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3))
                                + WLAN_PROBERESP_OFF_CAP_INFO);

    // Information elements
    pItem = (PWLAN_IE)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3))
                       + WLAN_PROBERESP_OFF_SSID);

    while( ((PBYTE)pItem) < (pFrame->pBuf + pFrame->len) ) {
        switch (pItem->byElementID) {
            case WLAN_EID_SSID:
                if (pFrame->pSSID == NULL)
                pFrame->pSSID = (PWLAN_IE_SSID)pItem;
                break;
            case WLAN_EID_SUPP_RATES:
                if (pFrame->pSuppRates == NULL)
                pFrame->pSuppRates = (PWLAN_IE_SUPP_RATES)pItem;
                break;
            case WLAN_EID_FH_PARMS:
                break;
            case WLAN_EID_DS_PARMS:
                if (pFrame->pDSParms == NULL)
                    pFrame->pDSParms = (PWLAN_IE_DS_PARMS)pItem;
                break;
            case WLAN_EID_CF_PARMS:
                if (pFrame->pCFParms == NULL)
                    pFrame->pCFParms = (PWLAN_IE_CF_PARMS)pItem;
                break;
            case WLAN_EID_IBSS_PARMS:
                if (pFrame->pIBSSParms == NULL)
                    pFrame->pIBSSParms = (PWLAN_IE_IBSS_PARMS)pItem;
                break;

            case WLAN_EID_RSN:
                if (pFrame->pRSN == NULL) {
                    pFrame->pRSN = (PWLAN_IE_RSN)pItem;
                }
                break;
            case WLAN_EID_RSN_WPA:
                if (pFrame->pRSNWPA == NULL) {
                    if (WPAb_Is_RSN((PWLAN_IE_RSN_EXT)pItem) == TRUE)
                        pFrame->pRSNWPA = (PWLAN_IE_RSN_EXT)pItem;
                }
                break;
            case WLAN_EID_ERP:
                if (pFrame->pERP == NULL)
                    pFrame->pERP = (PWLAN_IE_ERP)pItem;
                break;
            case WLAN_EID_EXTSUPP_RATES:
                if (pFrame->pExtSuppRates == NULL)
                    pFrame->pExtSuppRates = (PWLAN_IE_SUPP_RATES)pItem;
                break;

            case WLAN_EID_COUNTRY:      //7
                if (pFrame->pIE_Country == NULL)
                    pFrame->pIE_Country = (PWLAN_IE_COUNTRY)pItem;
                break;

            case WLAN_EID_PWR_CONSTRAINT:   //32
                if (pFrame->pIE_PowerConstraint == NULL)
                    pFrame->pIE_PowerConstraint = (PWLAN_IE_PW_CONST)pItem;
                break;

            case WLAN_EID_CH_SWITCH:    //37
                if (pFrame->pIE_CHSW == NULL)
                    pFrame->pIE_CHSW = (PWLAN_IE_CH_SW)pItem;
                break;

            case WLAN_EID_QUIET:        //40
                if (pFrame->pIE_Quiet == NULL)
                    pFrame->pIE_Quiet = (PWLAN_IE_QUIET)pItem;
                break;

            case WLAN_EID_IBSS_DFS:
                if (pFrame->pIE_IBSSDFS == NULL)
                    pFrame->pIE_IBSSDFS = (PWLAN_IE_IBSS_DFS)pItem;
                break;

            default:
                DBG_PRT(MSG_LEVEL_DEBUG, KERN_INFO "Bad EID=%dd in proberesp\n", pItem->byElementID);
                break;
        }

        pItem = (PWLAN_IE)(((PBYTE)pItem) + 2 +  pItem->len);
    }
    return;
}



void
vMgrEncodeAuthen(
    PWLAN_FR_AUTHEN  pFrame
    )
{
    pFrame->pHdr = (PUWLAN_80211HDR)pFrame->pBuf;

    // Fixed Fields
    pFrame->pwAuthAlgorithm = (PWORD)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3))
                                      + WLAN_AUTHEN_OFF_AUTH_ALG);
    pFrame->pwAuthSequence = (PWORD)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3))
                                     + WLAN_AUTHEN_OFF_AUTH_SEQ);
    pFrame->pwStatus = (PWORD)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3))
                               + WLAN_AUTHEN_OFF_STATUS);
    pFrame->len = WLAN_HDR_ADDR3_LEN + WLAN_AUTHEN_OFF_STATUS + sizeof(*(pFrame->pwStatus));

    return;
}



void
vMgrDecodeAuthen(
    PWLAN_FR_AUTHEN  pFrame
    )
{
    PWLAN_IE    pItem;

    pFrame->pHdr = (PUWLAN_80211HDR)pFrame->pBuf;

    // Fixed Fields
    pFrame->pwAuthAlgorithm = (PWORD)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3))
                                      + WLAN_AUTHEN_OFF_AUTH_ALG);
    pFrame->pwAuthSequence = (PWORD)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3))
                                     + WLAN_AUTHEN_OFF_AUTH_SEQ);
    pFrame->pwStatus = (PWORD)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3))
                               + WLAN_AUTHEN_OFF_STATUS);

    // Information elements
    pItem = (PWLAN_IE)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3))
                       + WLAN_AUTHEN_OFF_CHALLENGE);

    if ((((PBYTE)pItem) < (pFrame->pBuf + pFrame->len)) && (pItem->byElementID == WLAN_EID_CHALLENGE)) {
        pFrame->pChallenge = (PWLAN_IE_CHALLENGE)pItem;
    }

    return;
}



void
vMgrEncodeDeauthen(
    PWLAN_FR_DEAUTHEN  pFrame
    )
{
    pFrame->pHdr = (PUWLAN_80211HDR)pFrame->pBuf;

    // Fixed Fields
    pFrame->pwReason = (PWORD)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3))
                               + WLAN_DEAUTHEN_OFF_REASON);
    pFrame->len = WLAN_HDR_ADDR3_LEN + WLAN_DEAUTHEN_OFF_REASON + sizeof(*(pFrame->pwReason));

    return;
}



void
vMgrDecodeDeauthen(
    PWLAN_FR_DEAUTHEN  pFrame
    )
{
    pFrame->pHdr = (PUWLAN_80211HDR)pFrame->pBuf;

    // Fixed Fields
    pFrame->pwReason = (PWORD)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3))
                               + WLAN_DEAUTHEN_OFF_REASON);

    return;
}



void
vMgrEncodeReassocResponse(
    PWLAN_FR_REASSOCRESP  pFrame
     )
{
    pFrame->pHdr = (PUWLAN_80211HDR)pFrame->pBuf;

    // Fixed Fields
    pFrame->pwCapInfo = (PWORD)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3))
                                + WLAN_REASSOCRESP_OFF_CAP_INFO);
    pFrame->pwStatus = (PWORD)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3))
                               + WLAN_REASSOCRESP_OFF_STATUS);
    pFrame->pwAid = (PWORD)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3))
                            + WLAN_REASSOCRESP_OFF_AID);

    pFrame->len = WLAN_HDR_ADDR3_LEN + WLAN_REASSOCRESP_OFF_AID + sizeof(*(pFrame->pwAid));

    return;
}




void
vMgrDecodeReassocResponse(
    PWLAN_FR_REASSOCRESP  pFrame
     )
{
    PWLAN_IE   pItem;

    pFrame->pHdr = (PUWLAN_80211HDR)pFrame->pBuf;

    // Fixed Fields
    pFrame->pwCapInfo = (PWORD)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3))
                                + WLAN_REASSOCRESP_OFF_CAP_INFO);
    pFrame->pwStatus = (PWORD)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3))
                               + WLAN_REASSOCRESP_OFF_STATUS);
    pFrame->pwAid = (PWORD)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3))
                            + WLAN_REASSOCRESP_OFF_AID);

    //Information elements
    pFrame->pSuppRates = (PWLAN_IE_SUPP_RATES)(WLAN_HDR_A3_DATA_PTR(&(pFrame->pHdr->sA3))
                                               + WLAN_REASSOCRESP_OFF_SUPP_RATES);

    pItem = (PWLAN_IE)(pFrame->pSuppRates);
    pItem = (PWLAN_IE)(((PBYTE)pItem) + 2 + pItem->len);

    if ((((PBYTE)pItem) < (pFrame->pBuf + pFrame->len)) && (pItem->byElementID == WLAN_EID_EXTSUPP_RATES)) {
        pFrame->pExtSuppRates = (PWLAN_IE_SUPP_RATES)pItem;
    }
    return;
}
