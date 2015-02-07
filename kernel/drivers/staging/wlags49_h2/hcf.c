
//   vim:tw=110:ts=4:



#include "hcf.h"				// HCF and MSF common include file
#include "hcfdef.h"				// HCF specific include file
#include "mmd.h"				// MoreModularDriver common include file

#if ! defined offsetof
#define offsetof(s,m)   ((unsigned int)&(((s *)0)->m))
#endif // offsetof


/***********************************************************************************************************/
/***************************************  PROTOTYPES  ******************************************************/
/***********************************************************************************************************/
HCF_STATIC int			cmd_exe( IFBP ifbp, hcf_16 cmd_code, hcf_16 par_0 );
HCF_STATIC int			init( IFBP ifbp );
HCF_STATIC int			put_info( IFBP ifbp, LTVP ltvp );
#if (HCF_EXT) & HCF_EXT_MB
HCF_STATIC int			put_info_mb( IFBP ifbp, CFG_MB_INFO_STRCT FAR * ltvp );
#endif // HCF_EXT_MB
#if (HCF_TYPE) & HCF_TYPE_WPA
HCF_STATIC void			calc_mic( hcf_32* p, hcf_32 M );
void 					calc_mic_rx_frag( IFBP ifbp, wci_bufp p, int len );
void 					calc_mic_tx_frag( IFBP ifbp, wci_bufp p, int len );
HCF_STATIC int			check_mic( IFBP ifbp );
#endif // HCF_TYPE_WPA

HCF_STATIC void			calibrate( IFBP ifbp );
HCF_STATIC int			cmd_cmpl( IFBP ifbp );
HCF_STATIC hcf_16		get_fid( IFBP ifbp );
HCF_STATIC void			isr_info( IFBP ifbp );
#if HCF_DMA
HCF_STATIC DESC_STRCT*	get_frame_lst(IFBP ifbp, int tx_rx_flag);
#endif // HCF_DMA
HCF_STATIC void			get_frag( IFBP ifbp, wci_bufp bufp, int len BE_PAR( int word_len ) );	//char*, byte count (usually even)
#if HCF_DMA
HCF_STATIC void			put_frame_lst( IFBP ifbp, DESC_STRCT *descp, int tx_rx_flag );
#endif // HCF_DMA
HCF_STATIC void			put_frag( IFBP ifbp, wci_bufp bufp, int len BE_PAR( int word_len ) );
HCF_STATIC void			put_frag_finalize( IFBP ifbp );
HCF_STATIC int			setup_bap( IFBP ifbp, hcf_16 fid, int offset, int type );
#if (HCF_ASSERT) & HCF_ASSERT_PRINTF
static int fw_printf(IFBP ifbp, CFG_FW_PRINTF_STRCT FAR *ltvp);
#endif // HCF_ASSERT_PRINTF

HCF_STATIC int			download( IFBP ifbp, CFG_PROG_STRCT FAR *ltvp );
#if (HCF_ENCAP) & HCF_ENC
HCF_STATIC hcf_8		hcf_encap( wci_bufp type );
#endif // HCF_ENCAP
HCF_STATIC hcf_8		null_addr[4] = { 0, 0, 0, 0 };
#if ! defined IN_PORT_WORD			//replace I/O Macros with logging facility
extern FILE *log_file;

#define IN_PORT_WORD(port)			in_port_word( (hcf_io)(port) )

static hcf_16 in_port_word( hcf_io port ) {
hcf_16 i = (hcf_16)_inpw( port );
	if ( log_file ) {
		fprintf( log_file, "\nR %2.2x %4.4x", (port)&0xFF, i);
	}
	return i;
} // in_port_word

#define OUT_PORT_WORD(port, value)	out_port_word( (hcf_io)(port), (hcf_16)(value) )

static void out_port_word( hcf_io port, hcf_16 value ) {
	_outpw( port, value );
	if ( log_file ) {
		fprintf( log_file, "\nW %2.02x %4.04x", (port)&0xFF, value );
	}
}

void IN_PORT_STRING_32( hcf_io prt, hcf_32 FAR * dst, int n)	{
	int i = 0;
	hcf_16 FAR * p;
	if ( log_file ) {
		fprintf( log_file, "\nread string_32 length %04x (%04d) at port %02.2x to addr %lp",
				 (hcf_16)n, (hcf_16)n, (hcf_16)(prt)&0xFF, dst);
	}
	while ( n-- ) {
		p = (hcf_16 FAR *)dst;
		*p++ = (hcf_16)_inpw( prt );
		*p   = (hcf_16)_inpw( prt );
		if ( log_file ) {
			fprintf( log_file, "%s%08lx ", i++ % 0x08 ? " " : "\n", *dst);
		}
		dst++;
	}
} // IN_PORT_STRING_32

void IN_PORT_STRING_8_16( hcf_io prt, hcf_8 FAR * dst, int n) {	//also handles byte alignment problems
	hcf_16 FAR * p = (hcf_16 FAR *)dst;							//this needs more elaborate code in non-x86 platforms
	int i = 0;
	if ( log_file ) {
		fprintf( log_file, "\nread string_16 length %04x (%04d) at port %02.2x to addr %lp",
				 (hcf_16)n, (hcf_16)n, (hcf_16)(prt)&0xFF, dst );
	}
	while ( n-- ) {
		*p =(hcf_16)_inpw( prt);
		if ( log_file ) {
			if ( i++ % 0x10 ) {
				fprintf( log_file, "%04x ", *p);
			} else {
				fprintf( log_file, "\n%04x ", *p);
			}
		}
		p++;
	}
} // IN_PORT_STRING_8_16

void OUT_PORT_STRING_32( hcf_io prt, hcf_32 FAR * src, int n)	{
	int i = 0;
	hcf_16 FAR * p;
	if ( log_file ) {
		fprintf( log_file, "\nwrite string_32 length %04x (%04d) at port %02.2x",
				 (hcf_16)n, (hcf_16)n, (hcf_16)(prt)&0xFF);
	}
	while ( n-- ) {
		p = (hcf_16 FAR *)src;
		_outpw( prt, *p++ );
		_outpw( prt, *p   );
		if ( log_file ) {
			fprintf( log_file, "%s%08lx ", i++ % 0x08 ? " " : "\n", *src);
		}
		src++;
	}
} // OUT_PORT_STRING_32

void OUT_PORT_STRING_8_16( hcf_io prt, hcf_8 FAR * src, int n)	{	//also handles byte alignment problems
	hcf_16 FAR * p = (hcf_16 FAR *)src;								//this needs more elaborate code in non-x86 platforms
	int i = 0;
	if ( log_file ) {
		fprintf( log_file, "\nwrite string_16 length %04x (%04d) at port %04x", n, n, (hcf_16)prt);
	}
	while ( n-- ) {
		(void)_outpw( prt, *p);
		if ( log_file ) {
			if ( i++ % 0x10 ) {
				fprintf( log_file, "%04x ", *p);
			} else {
				fprintf( log_file, "\n%04x ", *p);
			}
		}
		p++;
	}
} // OUT_PORT_STRING_8_16

#endif // IN_PORT_WORD


#if HCF_ASSERT
IFBP BASED assert_ifbp = NULL;			//to make asserts easily work under MMD and DHF
#endif // HCF_ASSERT

#if HCF_ENCAP
/* SNAP header to be inserted in Ethernet-II frames */
HCF_STATIC  hcf_8 BASED snap_header[] = { 0xAA, 0xAA, 0x03, 0x00, 0x00,	//5 bytes signature +
										  0 };							//1 byte protocol identifier
#endif // HCF_ENCAP

#if (HCF_TYPE) & HCF_TYPE_WPA
HCF_STATIC hcf_8 BASED mic_pad[8] = { 0x5A, 0, 0, 0, 0, 0, 0, 0 };		//MIC padding of message
#endif // HCF_TYPE_WPA

#if defined MSF_COMPONENT_ID
CFG_IDENTITY_STRCT BASED cfg_drv_identity = {
	sizeof(cfg_drv_identity)/sizeof(hcf_16) - 1,	//length of RID
	CFG_DRV_IDENTITY,			// (0x0826)
	MSF_COMPONENT_ID,
	MSF_COMPONENT_VAR,
	MSF_COMPONENT_MAJOR_VER,
	MSF_COMPONENT_MINOR_VER
} ;

CFG_RANGES_STRCT BASED cfg_drv_sup_range = {
	sizeof(cfg_drv_sup_range)/sizeof(hcf_16) - 1,	//length of RID
	CFG_DRV_SUP_RANGE,			// (0x0827)

	COMP_ROLE_SUPL,
	COMP_ID_DUI,
	{{	DUI_COMPAT_VAR,
		DUI_COMPAT_BOT,
		DUI_COMPAT_TOP
	}}
} ;

struct CFG_RANGE3_STRCT BASED cfg_drv_act_ranges_pri = {
	sizeof(cfg_drv_act_ranges_pri)/sizeof(hcf_16) - 1,	//length of RID
	CFG_DRV_ACT_RANGES_PRI,		// (0x0828)

	COMP_ROLE_ACT,
	COMP_ID_PRI,
	{
	 { 0, 0, 0 }, 							// HCF_PRI_VAR_1 not supported by HCF 7
	 { 0, 0, 0 }, 							// HCF_PRI_VAR_2 not supported by HCF 7
	 {	3,									//var_rec[2] - Variant number
		CFG_DRV_ACT_RANGES_PRI_3_BOTTOM,		//		 - Bottom Compatibility
		CFG_DRV_ACT_RANGES_PRI_3_TOP			//		 - Top Compatibility
	 }
	}
} ;


struct CFG_RANGE4_STRCT BASED cfg_drv_act_ranges_sta = {
	sizeof(cfg_drv_act_ranges_sta)/sizeof(hcf_16) - 1,	//length of RID
	CFG_DRV_ACT_RANGES_STA,		// (0x0829)

	COMP_ROLE_ACT,
	COMP_ID_STA,
	{
#if defined HCF_STA_VAR_1
	 {	1,									//var_rec[1] - Variant number
		CFG_DRV_ACT_RANGES_STA_1_BOTTOM,		//		 - Bottom Compatibility
		CFG_DRV_ACT_RANGES_STA_1_TOP			//		 - Top Compatibility
	 },
#else
	 { 0, 0, 0 },
#endif // HCF_STA_VAR_1
#if defined HCF_STA_VAR_2
	 {	2,									//var_rec[1] - Variant number
		CFG_DRV_ACT_RANGES_STA_2_BOTTOM,		//		 - Bottom Compatibility
		CFG_DRV_ACT_RANGES_STA_2_TOP			//		 - Top Compatibility
	 },
#else
	 { 0, 0, 0 },
#endif // HCF_STA_VAR_2
// For Native_USB (Not used!)
#if defined HCF_STA_VAR_3
	 {	3,									//var_rec[1] - Variant number
		CFG_DRV_ACT_RANGES_STA_3_BOTTOM,		//		 - Bottom Compatibility
		CFG_DRV_ACT_RANGES_STA_3_TOP			//		 - Top Compatibility
	 },
#else
	 { 0, 0, 0 },
#endif // HCF_STA_VAR_3
// Warp
#if defined HCF_STA_VAR_4
	 {	4,									//var_rec[1] - Variant number
		CFG_DRV_ACT_RANGES_STA_4_BOTTOM,		//           - Bottom Compatibility
		CFG_DRV_ACT_RANGES_STA_4_TOP			//           - Top Compatibility
	 }
#else
	 { 0, 0, 0 }
#endif // HCF_STA_VAR_4
	}
} ;


struct CFG_RANGE6_STRCT BASED cfg_drv_act_ranges_hsi = {
	sizeof(cfg_drv_act_ranges_hsi)/sizeof(hcf_16) - 1,	//length of RID
	CFG_DRV_ACT_RANGES_HSI,		// (0x082A)
	COMP_ROLE_ACT,
	COMP_ID_HSI,
	{
#if defined HCF_HSI_VAR_0					// Controlled deployment
	 {	0,									// var_rec[1] - Variant number
		CFG_DRV_ACT_RANGES_HSI_0_BOTTOM,		//           - Bottom Compatibility
		CFG_DRV_ACT_RANGES_HSI_0_TOP			//           - Top Compatibility
	 },
#else
	 { 0, 0, 0 },
#endif // HCF_HSI_VAR_0
	 { 0, 0, 0 }, 							// HCF_HSI_VAR_1 not supported by HCF 7
	 { 0, 0, 0 }, 							// HCF_HSI_VAR_2 not supported by HCF 7
	 { 0, 0, 0 }, 							// HCF_HSI_VAR_3 not supported by HCF 7
#if defined HCF_HSI_VAR_4					// Hermes-II all types
	 {	4,									// var_rec[1] - Variant number
		CFG_DRV_ACT_RANGES_HSI_4_BOTTOM,		//           - Bottom Compatibility
		CFG_DRV_ACT_RANGES_HSI_4_TOP			//           - Top Compatibility
	 },
#else
	 { 0, 0, 0 },
#endif // HCF_HSI_VAR_4
#if defined HCF_HSI_VAR_5					// WARP Hermes-2.5
	 {	5,									// var_rec[1] - Variant number
		CFG_DRV_ACT_RANGES_HSI_5_BOTTOM,		//           - Bottom Compatibility
		CFG_DRV_ACT_RANGES_HSI_5_TOP			//           - Top Compatibility
	 }
#else
	 { 0, 0, 0 }
#endif // HCF_HSI_VAR_5
	}
} ;


CFG_RANGE4_STRCT BASED cfg_drv_act_ranges_apf = {
	sizeof(cfg_drv_act_ranges_apf)/sizeof(hcf_16) - 1,	//length of RID
	CFG_DRV_ACT_RANGES_APF,		// (0x082B)

	COMP_ROLE_ACT,
	COMP_ID_APF,
	{
#if defined HCF_APF_VAR_1				//(Fake) Hermes-I
	 {	1,									//var_rec[1] - Variant number
		CFG_DRV_ACT_RANGES_APF_1_BOTTOM,		//           - Bottom Compatibility
		CFG_DRV_ACT_RANGES_APF_1_TOP			//           - Top Compatibility
	 },
#else
	 { 0, 0, 0 },
#endif // HCF_APF_VAR_1
#if defined HCF_APF_VAR_2				//Hermes-II
	 {	2,									// var_rec[1] - Variant number
		CFG_DRV_ACT_RANGES_APF_2_BOTTOM,		//           - Bottom Compatibility
		CFG_DRV_ACT_RANGES_APF_2_TOP			//           - Top Compatibility
	 },
#else
	 { 0, 0, 0 },
#endif // HCF_APF_VAR_2
#if defined HCF_APF_VAR_3						// Native_USB
	 {	3,										// var_rec[1] - Variant number
		CFG_DRV_ACT_RANGES_APF_3_BOTTOM,		//           - Bottom Compatibility	!!!!!see note below!!!!!!!
		CFG_DRV_ACT_RANGES_APF_3_TOP			//           - Top Compatibility
	 },
#else
	 { 0, 0, 0 },
#endif // HCF_APF_VAR_3
#if defined HCF_APF_VAR_4						// WARP Hermes 2.5
	 {	4,										// var_rec[1] - Variant number
		CFG_DRV_ACT_RANGES_APF_4_BOTTOM,		//           - Bottom Compatibility	!!!!!see note below!!!!!!!
		CFG_DRV_ACT_RANGES_APF_4_TOP			//           - Top Compatibility
	 }
#else
	 { 0, 0, 0 }
#endif // HCF_APF_VAR_4
	}
} ;
#define HCF_VERSION  TEXT( "HCF$Revision: 1.10 $" )

static struct /*CFG_HCF_OPT_STRCT*/ {
	hcf_16	len;					//length of cfg_hcf_opt struct
	hcf_16	typ;					//type 0x082C
	hcf_16	 v0;						//offset HCF_VERSION
	hcf_16	 v1;						// MSF_COMPONENT_ID
	hcf_16	 v2;						// HCF_ALIGN
	hcf_16	 v3;						// HCF_ASSERT
	hcf_16	 v4;						// HCF_BIG_ENDIAN
	hcf_16	 v5;						// /* HCF_DLV | HCF_DLNV */
	hcf_16	 v6;						// HCF_DMA
	hcf_16	 v7;						// HCF_ENCAP
	hcf_16	 v8;						// HCF_EXT
	hcf_16	 v9;						// HCF_INT_ON
	hcf_16	v10;						// HCF_IO
	hcf_16	v11;						// HCF_LEGACY
	hcf_16	v12;						// HCF_MAX_LTV
	hcf_16	v13;						// HCF_PROT_TIME
	hcf_16	v14;						// HCF_SLEEP
	hcf_16	v15;						// HCF_TALLIES
	hcf_16	v16;						// HCF_TYPE
	hcf_16	v17;						// HCF_NIC_TAL_CNT
	hcf_16	v18;						// HCF_HCF_TAL_CNT
	hcf_16	v19;						// offset tallies
	TCHAR	val[sizeof(HCF_VERSION)];
} BASED cfg_hcf_opt = {
	sizeof(cfg_hcf_opt)/sizeof(hcf_16) -1,
	CFG_HCF_OPT,				// (0x082C)
	( sizeof(cfg_hcf_opt) - sizeof(HCF_VERSION) - 4 )/sizeof(hcf_16),
#if defined MSF_COMPONENT_ID
	MSF_COMPONENT_ID,
#else
	0,
#endif // MSF_COMPONENT_ID
	HCF_ALIGN,
	HCF_ASSERT,
	HCF_BIG_ENDIAN,
	0,									// /* HCF_DLV | HCF_DLNV*/,
	HCF_DMA,
	HCF_ENCAP,
	HCF_EXT,
	HCF_INT_ON,
	HCF_IO,
	HCF_LEGACY,
	HCF_MAX_LTV,
	HCF_PROT_TIME,
	HCF_SLEEP,
	HCF_TALLIES,
	HCF_TYPE,
#if (HCF_TALLIES) & ( HCF_TALLIES_NIC | HCF_TALLIES_HCF )
	HCF_NIC_TAL_CNT,
	HCF_HCF_TAL_CNT,
	offsetof(IFB_STRCT, IFB_TallyLen ),
#else
	0, 0, 0,
#endif // HCF_TALLIES_NIC / HCF_TALLIES_HCF
	HCF_VERSION
}; // cfg_hcf_opt
#endif // MSF_COMPONENT_ID

#if defined HCF_TALLIES_EXTRA
	replaced by HCF_EXT_TALLIES_FW ;
#endif // HCF_TALLIES_EXTRA

#if defined MSF_COMPONENT_ID || (HCF_EXT) & HCF_EXT_MB
#if (HCF_EXT) & HCF_EXT_MB
HCF_STATIC LTV_STRCT BASED cfg_null = { 1, CFG_NULL, {0} };
#endif // HCF_EXT_MB
HCF_STATIC hcf_16* BASED xxxx[ ] = {
#if (HCF_EXT) & HCF_EXT_MB
	&cfg_null.len,							//CFG_NULL						0x0820
#endif // HCF_EXT_MB
#if defined MSF_COMPONENT_ID
	&cfg_drv_identity.len,					//CFG_DRV_IDENTITY              0x0826
	&cfg_drv_sup_range.len,					//CFG_DRV_SUP_RANGE             0x0827
	&cfg_drv_act_ranges_pri.len,			//CFG_DRV_ACT_RANGES_PRI        0x0828
	&cfg_drv_act_ranges_sta.len,			//CFG_DRV_ACT_RANGES_STA		0x0829
	&cfg_drv_act_ranges_hsi.len,			//CFG_DRV_ACT_RANGES_HSI		0x082A
	&cfg_drv_act_ranges_apf.len,			//CFG_DRV_ACT_RANGES_APF		0x082B
	&cfg_hcf_opt.len,						//CFG_HCF_OPT					0x082C
	NULL,									//IFB_PRIIdentity placeholder	0xFD02
	NULL,									//IFB_PRISup placeholder		0xFD03
#endif // MSF_COMPONENT_ID
	NULL									//endsentinel
  };
#define xxxx_PRI_IDENTITY_OFFSET	(sizeof(xxxx)/sizeof(xxxx[0]) - 3)

#endif // MSF_COMPONENT_ID / HCF_EXT_MB



#if (HCF_DL_ONLY) == 0
#if ( (HCF_TYPE) & HCF_TYPE_HII5 ) == 0
#if CFG_SCAN != CFG_TALLIES - HCF_ACT_TALLIES + HCF_ACT_SCAN
err: "maintenance" apparently inviolated the underlying assumption about the numerical values of these macros
#endif
#endif // HCF_TYPE_HII5
#if CFG_PRS_SCAN != CFG_TALLIES - HCF_ACT_TALLIES + HCF_ACT_PRS_SCAN
err: "maintenance" apparently inviolated the underlying assumption about the numerical values of these macros
#endif
int
hcf_action( IFBP ifbp, hcf_16 action )
{
int	rc = HCF_SUCCESS;

	HCFASSERT( ifbp->IFB_Magic == HCF_MAGIC, ifbp->IFB_Magic )
#if HCF_INT_ON
	HCFLOGENTRY( action == HCF_ACT_INT_FORCE_ON ? HCF_TRACE_ACTION_KLUDGE : HCF_TRACE_ACTION, action )														/* 0 */
#if (HCF_SLEEP)
	HCFASSERT( ifbp->IFB_IntOffCnt != 0xFFFE || action == HCF_ACT_INT_OFF,
			   MERGE_2( action, ifbp->IFB_IntOffCnt ) )
#else
	HCFASSERT( ifbp->IFB_IntOffCnt != 0xFFFE, action )
#endif // HCF_SLEEP
	HCFASSERT( ifbp->IFB_IntOffCnt != 0xFFFF ||
			   action == HCF_ACT_INT_OFF || action == HCF_ACT_INT_FORCE_ON,  action )
	HCFASSERT( ifbp->IFB_IntOffCnt <= 16 || ifbp->IFB_IntOffCnt >= 0xFFFE,
			   MERGE_2( action, ifbp->IFB_IntOffCnt ) )	//nesting more than 16 deep seems unreasonable
#endif // HCF_INT_ON

	switch (action) {
#if HCF_INT_ON
hcf_16	i;
	  case HCF_ACT_INT_OFF:						// Disable Interrupt generation
#if HCF_SLEEP
		if ( ifbp->IFB_IntOffCnt == 0xFFFE ) {	// WakeUp test	;?tie this to the "new" super-LinkStat
			ifbp->IFB_IntOffCnt++;						// restore conventional I/F
			OPW(HREG_IO, HREG_IO_WAKEUP_ASYNC );		// set wakeup bit
			OPW(HREG_IO, HREG_IO_WAKEUP_ASYNC );		// set wakeup bit to counteract the clearing by F/W
			// 800 us latency before FW switches to high power
			MSF_WAIT(800);								// MSF-defined function to wait n microseconds.
//OOR		if ( ifbp->IFB_DSLinkStat & CFG_LINK_STAT_DS_OOR ) { // OutOfRange
//				printk( "<5>ACT_INT_OFF: Deepsleep phase terminated, enable and go to AwaitConnection\n" );		//;?remove me 1 day
//				hcf_cntl( ifbp, HCF_CNTL_ENABLE );
//			}
//			ifbp->IFB_DSLinkStat &= ~( CFG_LINK_STAT_DS_IR | CFG_LINK_STAT_DS_OOR);	//clear IR/OOR state
		}
#endif // HCF_SLEEP
/*2*/	ifbp->IFB_IntOffCnt++;
//!     rc = 0;
		i = IPW( HREG_INT_EN );
		OPW( HREG_INT_EN, 0 );
		if ( i & 0x1000 ) {
			rc = HCF_ERR_NO_NIC;
		} else {
			if ( i & IPW( HREG_EV_STAT ) ) {
				rc = HCF_INT_PENDING;
			}
		}
		break;

	  case HCF_ACT_INT_FORCE_ON:				// Enforce Enable Interrupt generation
		ifbp->IFB_IntOffCnt = 0;
		//Fall through in HCF_ACT_INT_ON

	  case HCF_ACT_INT_ON:						// Enable Interrupt generation
/*4*/	if ( ifbp->IFB_IntOffCnt-- == 0 && ifbp->IFB_CardStat == 0 ) {
												//determine Interrupt Event mask
#if HCF_DMA
			if ( ifbp->IFB_CntlOpt & USE_DMA ) {
				i = HREG_EV_INFO | HREG_EV_RDMAD | HREG_EV_TDMAD | HREG_EV_TX_EXT;	//mask when DMA active
			} else
#endif // HCF_DMA
			{
				i = HREG_EV_INFO | HREG_EV_RX | HREG_EV_TX_EXT;						//mask when DMA not active
				if ( ifbp->IFB_RscInd == 0 ) {
					i |= HREG_EV_ALLOC;												//mask when no TxFID available
				}
			}
#if HCF_SLEEP
			if ( ( IPW(HREG_EV_STAT) & ( i | HREG_EV_SLEEP_REQ ) ) == HREG_EV_SLEEP_REQ ) {
				// firmware indicates it would like to go into sleep modus
				// only acknowledge this request if no other events that can cause an interrupt are pending
				ifbp->IFB_IntOffCnt--;			//becomes 0xFFFE
            	OPW( HREG_INT_EN, i | HREG_EV_TICK );
				OPW( HREG_EV_ACK, HREG_EV_SLEEP_REQ | HREG_EV_TICK | HREG_EV_ACK_REG_READY );
			} else
#endif // HCF_SLEEP
			{
            	OPW( HREG_INT_EN, i | HREG_EV_SLEEP_REQ );
			}
		}
		break;
#endif // HCF_INT_ON

#if (HCF_SLEEP) & HCF_DDS
	  case HCF_ACT_SLEEP:						// DDS Sleep request
		hcf_cntl( ifbp, HCF_CNTL_DISABLE );
		cmd_exe( ifbp, HCMD_SLEEP, 0 );
		break;
// 	  case HCF_ACT_WAKEUP:						// DDS Wakeup request
// 		HCFASSERT( ifbp->IFB_IntOffCnt == 0xFFFE, ifbp->IFB_IntOffCnt )
// 		ifbp->IFB_IntOffCnt++;					// restore conventional I/F
// 		OPW( HREG_IO, HREG_IO_WAKEUP_ASYNC );
// 		MSF_WAIT(800);							// MSF-defined function to wait n microseconds.
// 		rc = hcf_action( ifbp, HCF_ACT_INT_OFF );	/*bogus, IFB_IntOffCnt == 0xFFFF, so if you carefully look
// 													 *at the #if HCF_DDS statements, HCF_ACT_INT_OFF is empty
// 													 *for DDS. "Much" better would be to merge the flows for
// 													 *DDS and DEEP_SLEEP
// 													 */
// 		break;
#endif // HCF_DDS

#if (HCF_TYPE) & HCF_TYPE_CCX
	  case HCF_ACT_CCX_ON:						// enable CKIP
	  case HCF_ACT_CCX_OFF:						// disable CKIP
		ifbp->IFB_CKIPStat = action;
		break;
#endif // HCF_TYPE_CCX

	  case HCF_ACT_RX_ACK:						//Receiver ACK
/*6*/	if ( ifbp->IFB_RxFID ) {
			DAWA_ACK( HREG_EV_RX );
		}
		ifbp->IFB_RxFID = ifbp->IFB_RxLen = 0;
		break;

/*8*/ case	HCF_ACT_PRS_SCAN:					// Hermes PRS Scan (F102)
		OPW( HREG_PARAM_1, 0x3FFF );
			//Fall through in HCF_ACT_TALLIES
	  case HCF_ACT_TALLIES:						// Hermes Inquire Tallies (F100)
#if ( (HCF_TYPE) & HCF_TYPE_HII5 ) == 0
	  case HCF_ACT_SCAN:						// Hermes Inquire Scan (F101)
#endif // HCF_TYPE_HII5
		/*!! the assumptions about numerical relationships between CFG_TALLIES etc and HCF_ACT_TALLIES etc
		 *   are checked by #if statements just prior to this routine resulting in: err "maintenance"	*/
		cmd_exe( ifbp, HCMD_INQUIRE, action - HCF_ACT_TALLIES + CFG_TALLIES );
		break;

	  default:
		HCFASSERT( DO_ASSERT, action )
		break;
	}
	//! do not HCFASSERT( rc == HCF_SUCCESS, rc )														/* 30*/
	HCFLOGEXIT( HCF_TRACE_ACTION )
	return rc;
} // hcf_action
#endif // HCF_DL_ONLY


int
hcf_cntl( IFBP ifbp, hcf_16 cmd )
{
int	rc = HCF_ERR_INCOMP_FW;
#if HCF_ASSERT
{	int x = cmd & HCMD_CMD_CODE;
	if ( x == HCF_CNTL_CONTINUE ) x &= ~HCMD_RETRY;
	else if ( (x == HCMD_DISABLE || x == HCMD_ENABLE) && ifbp->IFB_FWIdentity.comp_id == COMP_ID_FW_AP ) {
		x &= ~HFS_TX_CNTL_PORT;
	}
	HCFASSERT( x==HCF_CNTL_ENABLE  || x==HCF_CNTL_DISABLE    || HCF_CNTL_CONTINUE ||
			   x==HCF_CNTL_CONNECT || x==HCF_CNTL_DISCONNECT, cmd )
}
#endif // HCF_ASSERT
// #if (HCF_SLEEP) & HCF_DDS
// 	HCFASSERT( ifbp->IFB_IntOffCnt != 0xFFFE, cmd )
// #endif // HCF_DDS
	HCFLOGENTRY( HCF_TRACE_CNTL, cmd )
	if ( ifbp->IFB_CardStat == 0 ) {																 /*2*/
/*6*/	rc = cmd_exe( ifbp, cmd, 0 );
#if (HCF_SLEEP) & HCF_DDS
		ifbp->IFB_TickCnt = 0;				//start 2 second period (with 1 tick uncertanty)
#endif // HCF_DDS
	}
#if HCF_DMA
	//!rlav : note that this piece of code is always executed, regardless of the DEFUNCT bit in IFB_CardStat.
	// The reason behind this is that the MSF should be able to get all its DMA resources back from the HCF,
	// even if the hardware is disfunctional. Practical example under Windows : surprise removal.
	if ( ifbp->IFB_CntlOpt & USE_DMA ) {
		hcf_io io_port = ifbp->IFB_IOBase;
		DESC_STRCT *p;
		if ( cmd == HCF_CNTL_DISABLE || cmd == HCF_CNTL_ENABLE ) {
			OUT_PORT_DWORD( (io_port + HREG_DMA_CTRL), DMA_CTRLSTAT_RESET);						/*8*/
			ifbp->IFB_CntlOpt &= ~DMA_ENABLED;
		}
		if ( cmd == HCF_CNTL_ENABLE ) {
			OUT_PORT_DWORD( (io_port + HREG_DMA_CTRL), DMA_CTRLSTAT_GO);
			/* ;? by rewriting hcf_dma_rx_put you can probably just call hcf_dma_rx_put( ifbp->IFB_FirstDesc[DMA_RX] )
			 * as additional beneficiary side effect, the SOP and EOP bits will also be cleared
			 */
			ifbp->IFB_CntlOpt |= DMA_ENABLED;
			HCFASSERT( NT_ASSERT, NEVER_TESTED )
			// make the entire rx descriptor chain DMA-owned, so the DMA engine can (re-)use it.
			p = ifbp->IFB_FirstDesc[DMA_RX];
			if (p != NULL) {   //;? Think this over again in the light of the new chaining strategy
				if ( 1 ) 	{ //begin alternative
					HCFASSERT( NT_ASSERT, NEVER_TESTED )
					put_frame_lst( ifbp, ifbp->IFB_FirstDesc[DMA_RX], DMA_RX );
					if ( ifbp->IFB_FirstDesc[DMA_RX] ) {
						put_frame_lst( ifbp, ifbp->IFB_FirstDesc[DMA_RX]->next_desc_addr, DMA_RX );
					}
				} else {
					while ( p ) {
						//p->buf_cntl.cntl_stat |= DESC_DMA_OWNED;
						p->BUF_CNT |= DESC_DMA_OWNED;
						p = p->next_desc_addr;
					}
					// a rx chain is available so hand it over to the DMA engine
					p = ifbp->IFB_FirstDesc[DMA_RX];
					OUT_PORT_DWORD( (io_port + HREG_RXDMA_PTR32), p->desc_phys_addr);
				}  //end alternative
			}
		}
	}
#endif // HCF_DMA
	HCFASSERT( rc == HCF_SUCCESS, rc )
	HCFLOGEXIT( HCF_TRACE_CNTL )
	return rc;
} // hcf_cntl


int
hcf_connect( IFBP ifbp, hcf_io io_base )
{
int			rc = HCF_SUCCESS;
hcf_io		io_addr;
hcf_32		prot_cnt;
hcf_8		*q;
LTV_STRCT	x;
#if HCF_ASSERT
	hcf_16 xa = ifbp->IFB_FWIdentity.typ;
	/* is assumed to cause an assert later on if hcf_connect is called without intervening hcf_disconnect.
	 * xa == CFG_FW_IDENTITY in subsequent calls without preceding hcf_disconnect,
	 * xa == 0 in subsequent calls with preceding hcf_disconnect,
	 * xa == "garbage" (any value except CFG_FW_IDENTITY is acceptable) in the initial call
	 */
#endif // HCF_ASSERT

	if ( io_base == HCF_DISCONNECT ) {					//disconnect
		io_addr = ifbp->IFB_IOBase;
		OPW( HREG_INT_EN, 0 );		//;?workaround against dying F/W on subsequent hcf_connect calls
	} else {											//connect								/* 0 */
		io_addr = io_base;
	}

#if 0 //;? if a subsequent hcf_connect is preceeded by an hcf_disconnect the wakeup is not needed !!
#if HCF_SLEEP
    OUT_PORT_WORD( .....+HREG_IO, HREG_IO_WAKEUP_ASYNC ); 	    //OPW not yet useable
	MSF_WAIT(800);								// MSF-defined function to wait n microseconds.
	note that MSF_WAIT uses not yet defined!!!! IFB_IOBase and IFB_TickIni (via PROT_CNT_INI)
	so be carefull if this code is restored
#endif // HCF_SLEEP
#endif // 0

#if ( (HCF_TYPE) & HCF_TYPE_PRELOADED ) == 0	//switch clock back for SEEPROM access  !!!
	OUT_PORT_WORD( io_addr + HREG_CMD, HCMD_INI );  	    //OPW not yet useable
	prot_cnt = INI_TICK_INI;
	HCF_WAIT_WHILE( (IN_PORT_WORD( io_addr +  HREG_EV_STAT) & HREG_EV_CMD) == 0 );
	OUT_PORT_WORD( (io_addr + HREG_IO), HREG_IO_SRESET );	//OPW not yet useable					/* 2a*/
#endif // HCF_TYPE_PRELOADED
	for ( q = (hcf_8*)(&ifbp->IFB_Magic); q > (hcf_8*)ifbp; *--q = 0 ) /*NOP*/;						/* 4 */
	ifbp->IFB_Magic		= HCF_MAGIC;
	ifbp->IFB_Version	= IFB_VERSION;
#if defined MSF_COMPONENT_ID //a new IFB demonstrates how dirty the solution is
	xxxx[xxxx_PRI_IDENTITY_OFFSET] = NULL;		//IFB_PRIIdentity placeholder	0xFD02
	xxxx[xxxx_PRI_IDENTITY_OFFSET+1] = NULL;	//IFB_PRISup placeholder		0xFD03
#endif // MSF_COMPONENT_ID
#if (HCF_TALLIES) & ( HCF_TALLIES_NIC | HCF_TALLIES_HCF )
	ifbp->IFB_TallyLen = 1 + 2 * (HCF_NIC_TAL_CNT + HCF_HCF_TAL_CNT);	//convert # of Tallies to L value for LTV
	ifbp->IFB_TallyTyp = CFG_TALLIES;			//IFB_TallyTyp: set T value
#endif // HCF_TALLIES_NIC / HCF_TALLIES_HCF
	ifbp->IFB_IOBase	= io_addr;				//set IO_Base asap, so asserts via HREG_SW_2 don't harm
	ifbp->IFB_IORange	= HREG_IO_RANGE;
	ifbp->IFB_CntlOpt	= USE_16BIT;
#if HCF_ASSERT
	assert_ifbp = ifbp;
	ifbp->IFB_AssertLvl = 1;
#if (HCF_ASSERT) & HCF_ASSERT_LNK_MSF_RTN
	if ( io_base != HCF_DISCONNECT ) {
		ifbp->IFB_AssertRtn = (MSF_ASSERT_RTNP)msf_assert;											/* 6 */
	}
#endif // HCF_ASSERT_LNK_MSF_RTN
#if (HCF_ASSERT) & HCF_ASSERT_MB				//build the structure to pass the assert info to hcf_put_info
	ifbp->IFB_AssertStrct.len = sizeof(ifbp->IFB_AssertStrct)/sizeof(hcf_16) - 1;
	ifbp->IFB_AssertStrct.typ = CFG_MB_INFO;
	ifbp->IFB_AssertStrct.base_typ = CFG_MB_ASSERT;
	ifbp->IFB_AssertStrct.frag_cnt = 1;
	ifbp->IFB_AssertStrct.frag_buf[0].frag_len =
		( offsetof(IFB_STRCT, IFB_AssertLvl) - offsetof(IFB_STRCT, IFB_AssertLine) ) / sizeof(hcf_16);
	ifbp->IFB_AssertStrct.frag_buf[0].frag_addr = &ifbp->IFB_AssertLine;
#endif // HCF_ASSERT_MB
#endif // HCF_ASSERT
	IF_PROT_TIME( prot_cnt = ifbp->IFB_TickIni = INI_TICK_INI; )
#if ( (HCF_TYPE) & HCF_TYPE_PRELOADED ) == 0
	//!! No asserts before Reset-bit in HREG_IO is cleared
	OPW( HREG_IO, 0x0000 );						//OPW useable										/* 2b*/
	HCF_WAIT_WHILE( (IPW( HREG_EV_STAT) & HREG_EV_CMD) == 0 );
	IF_PROT_TIME( HCFASSERT( prot_cnt, IPW( HREG_EV_STAT) ) )
	IF_PROT_TIME( if ( prot_cnt ) prot_cnt = ifbp->IFB_TickIni; )
#endif // HCF_TYPE_PRELOADED
	//!! No asserts before Reset-bit in HREG_IO is cleared
	HCFASSERT( DO_ASSERT, MERGE_2( HCF_ASSERT, 0xCAF0 )	) //just to proof that the complete assert machinery is working
	HCFASSERT( xa != CFG_FW_IDENTITY, 0 )		// assert if hcf_connect is called without intervening hcf_disconnect.
	HCFASSERT( ((hcf_32)(void*)ifbp & (HCF_ALIGN-1) ) == 0, (hcf_32)(void*)ifbp )
	HCFASSERT( (io_addr & 0x003F) == 0, io_addr )
												//if Busy bit in Cmd register
	if (IPW( HREG_CMD ) & HCMD_BUSY ) {																/* 8 */
												//.  Ack all to unblock a (possibly) blocked cmd pipe line
		OPW( HREG_EV_ACK, ~HREG_EV_SLEEP_REQ );
												//.  Wait for Busy bit drop  in Cmd register
												//.  Wait for Cmd  bit raise in Ev  register
		HCF_WAIT_WHILE( ( IPW( HREG_CMD ) & HCMD_BUSY ) && (IPW( HREG_EV_STAT) & HREG_EV_CMD) == 0 );
		IF_PROT_TIME( HCFASSERT( prot_cnt, IPW( HREG_EV_STAT) ) ) /* if prot_cnt == 0, cmd_exe will fail, causing DEFUNCT */
	}
	OPW( HREG_EV_ACK, ~HREG_EV_SLEEP_REQ );
#if ( (HCF_TYPE) & HCF_TYPE_PRELOADED ) == 0														/*12*/
	(void)cmd_exe( ifbp, HCMD_INI, 0 );
#endif // HCF_TYPE_PRELOADED
if ( io_base != HCF_DISCONNECT ) {
		rc = init( ifbp );																			/*14*/
		if ( rc == HCF_SUCCESS ) {
			x.len = 2;
			x.typ = CFG_NIC_BUS_TYPE;
			(void)hcf_get_info( ifbp, &x );
			ifbp->IFB_BusType = x.val[0];
			//CFG_NIC_BUS_TYPE not supported -> default 32 bits/DMA, MSF has to overrule via CFG_CNTL_OPT
			if ( x.len == 0 || x.val[0] == 0x0002 || x.val[0] == 0x0003 ) {
#if (HCF_IO) & HCF_IO_32BITS
				ifbp->IFB_CntlOpt &= ~USE_16BIT;			//reset USE_16BIT
#endif // HCF_IO_32BITS
#if HCF_DMA
				ifbp->IFB_CntlOpt |= USE_DMA;				//SET DMA
#else
				ifbp->IFB_IORange = 0x40 /*i.s.o. HREG_IO_RANGE*/;
#endif // HCF_DMA
			}
		}
	} else HCFASSERT(  ( ifbp->IFB_Magic ^= HCF_MAGIC ) == 0, ifbp->IFB_Magic ) /*NOP*/;
	/* of above HCFASSERT only the side effect is needed, NOP in case HCFASSERT is dummy */
	ifbp->IFB_IOBase = io_base;																		/* 0*/
	return rc;
} // hcf_connect

#if HCF_DMA
HCF_STATIC DESC_STRCT*
get_frame_lst( IFBP ifbp, int tx_rx_flag )
{

DESC_STRCT *head = ifbp->IFB_FirstDesc[tx_rx_flag];
DESC_STRCT *copy, *p, *prev;

	HCFASSERT( tx_rx_flag == DMA_RX || tx_rx_flag == DMA_TX, tx_rx_flag )
								//if FrameList
	if ( head ) {
								//.  search for last descriptor of first FrameList
		p = prev = head;
		while ( ( p->BUF_SIZE & DESC_EOP ) == 0 && p->next_desc_addr ) {
			if ( ( ifbp->IFB_CntlOpt & DMA_ENABLED ) == 0 ) {	//clear control bits when disabled
				p->BUF_CNT &= DESC_CNT_MASK;
			}
			prev = p;
			p = p->next_desc_addr;
		}
								//.  if DMA enabled
		if ( ifbp->IFB_CntlOpt & DMA_ENABLED ) {
								//.  .  if last descriptor of FrameList is DMA owned
								//.  .  or if FrameList is single (DELWA) Descriptor
			if ( p->BUF_CNT & DESC_DMA_OWNED || head->next_desc_addr == NULL ) {
								//.  .  .  refuse to return FrameList to caller
				head = NULL;
			}
		}
	}
								//if returnable FrameList found
	if ( head ) {
								//.  if FrameList is single (DELWA) Descriptor (implies DMA disabled)
 		if ( head->next_desc_addr == NULL ) {
								//.  .  clear DescriptorList
			/*;?ifbp->IFB_LastDesc[tx_rx_flag] =*/ ifbp->IFB_FirstDesc[tx_rx_flag] = NULL;
								//.  else
		} else {
								//.  .  strip hardware-related bits from last descriptor
								//.  .  remove DELWA Descriptor from head of DescriptorList
			copy = head;
	 		head = head->next_desc_addr;
								//.   .  exchange first (Confined) and last (possibly imprisoned) Descriptor
			copy->buf_phys_addr = p->buf_phys_addr;
			copy->buf_addr = p->buf_addr;
			copy->BUF_SIZE = p->BUF_SIZE &= DESC_CNT_MASK;	//get rid of DESC_EOP and possibly DESC_SOP
			copy->BUF_CNT = p->BUF_CNT &= DESC_CNT_MASK;	//get rid of DESC_DMA_OWNED
#if (HCF_EXT) & HCF_DESC_STRCT_EXT
			copy->DESC_MSFSup = p->DESC_MSFSup;
#endif // HCF_DESC_STRCT_EXT
								//.  .  turn into a DELWA Descriptor
			p->buf_addr = NULL;
								//.  .  chain copy to prev											/* 8*/
			prev->next_desc_addr = copy;
								//.  .  detach remainder of the DescriptorList from FrameList
			copy->next_desc_addr = NULL;
			copy->next_desc_phys_addr = 0xDEAD0000; //! just to be nice, not really needed
								//.  .  save the new start (i.e. DELWA Descriptor) in IFB_FirstDesc
			ifbp->IFB_FirstDesc[tx_rx_flag] = p;
		}
								//.  strip DESC_SOP from first descriptor
		head->BUF_SIZE &= DESC_CNT_MASK;
		//head->BUF_CNT &= DESC_CNT_MASK;  get rid of DESC_DMA_OWNED
		head->next_desc_phys_addr = 0xDEAD0000; //! just to be nice, not really needed
	}
								//return the just detached FrameList (if any)
	return head;
} // get_frame_lst


HCF_STATIC void
put_frame_lst( IFBP ifbp, DESC_STRCT *descp, int tx_rx_flag )
{
	DESC_STRCT	*p = descp;
	hcf_16 port;

	HCFASSERT( ifbp->IFB_CntlOpt & USE_DMA, ifbp->IFB_CntlOpt) //only hcf_dma_tx_put must also be DMA_ENABLED
	HCFASSERT( tx_rx_flag == DMA_RX || tx_rx_flag == DMA_TX, tx_rx_flag )
	HCFASSERT( p , 0 )

	while ( p ) {
		HCFASSERT( ((hcf_32)p & 3 ) == 0, (hcf_32)p )
		HCFASSERT( (p->BUF_CNT & ~DESC_CNT_MASK) == 0, p->BUF_CNT )
		HCFASSERT( (p->BUF_SIZE & ~DESC_CNT_MASK) == 0, p->BUF_SIZE )
		p->BUF_SIZE &= DESC_CNT_MASK;					//!!this SHOULD be superfluous in case of correct MSF
		p->BUF_CNT &= tx_rx_flag == DMA_RX ? 0 : DESC_CNT_MASK;	//!!this SHOULD be superfluous in case of correct MSF
		p->BUF_CNT |= DESC_DMA_OWNED;
		if ( p->next_desc_addr ) {
//			HCFASSERT( p->buf_addr && p->buf_phys_addr  && p->BUF_SIZE && +/- p->BUF_SIZE, ... )
			HCFASSERT( p->next_desc_addr->desc_phys_addr, (hcf_32)p->next_desc_addr )
			p->next_desc_phys_addr = p->next_desc_addr->desc_phys_addr;
		} else {									//
			p->next_desc_phys_addr = 0;
			if ( p->buf_addr == NULL ) {			// DELWA Descriptor
				HCFASSERT( descp == p, (hcf_32)descp )	//singleton DescriptorList
				HCFASSERT( ifbp->IFB_FirstDesc[tx_rx_flag] == NULL, (hcf_32)ifbp->IFB_FirstDesc[tx_rx_flag])
				HCFASSERT( ifbp->IFB_LastDesc[tx_rx_flag] == NULL, (hcf_32)ifbp->IFB_LastDesc[tx_rx_flag])
				descp->BUF_CNT = 0; //&= ~DESC_DMA_OWNED;
				ifbp->IFB_FirstDesc[tx_rx_flag] = descp;
// part of alternative ifbp->IFB_LastDesc[tx_rx_flag] = ifbp->IFB_FirstDesc[tx_rx_flag] = descp;
													// if "recycling" a FrameList
													// (e.g. called from hcf_cntl( HCF_CNTL_ENABLE )
													// .  prepare for activation DMA controller
// part of alternative descp = descp->next_desc_addr;
			} else {								//a "real" FrameList, hand it over to the DMA engine
				HCFASSERT( ifbp->IFB_FirstDesc[tx_rx_flag], (hcf_32)descp )
				HCFASSERT( ifbp->IFB_LastDesc[tx_rx_flag], (hcf_32)descp )
				HCFASSERT( ifbp->IFB_LastDesc[tx_rx_flag]->next_desc_addr == NULL,
						   (hcf_32)ifbp->IFB_LastDesc[tx_rx_flag]->next_desc_addr)
//				p->buf_cntl.cntl_stat |= DESC_DMA_OWNED;
				ifbp->IFB_LastDesc[tx_rx_flag]->next_desc_addr = descp;
				ifbp->IFB_LastDesc[tx_rx_flag]->next_desc_phys_addr = descp->desc_phys_addr;
				port = HREG_RXDMA_PTR32;
				if ( tx_rx_flag ) {
					p->BUF_SIZE |= DESC_EOP;	// p points at the last descriptor in the caller-supplied descriptor chain
					descp->BUF_SIZE |= DESC_SOP;
					port = HREG_TXDMA_PTR32;
				}
				OUT_PORT_DWORD( (ifbp->IFB_IOBase + port), descp->desc_phys_addr );
			}
			ifbp->IFB_LastDesc[tx_rx_flag] = p;
		}
		p = p->next_desc_addr;
	}
} // put_frame_lst



DESC_STRCT*
hcf_dma_rx_get (IFBP ifbp)
{
DESC_STRCT *descp;	// pointer to start of FrameList

	descp = get_frame_lst( ifbp, DMA_RX );
	if ( descp && descp->buf_addr )  //!be aware of the missing curly bracket

											//skip decapsulation at confined descriptor
#if (HCF_ENCAP) == HCF_ENC
#if (HCF_TYPE) & HCF_TYPE_CCX
	if ( ifbp->IFB_CKIPStat == HCF_ACT_CCX_OFF )
#endif // HCF_TYPE_CCX
    {
int i;
DESC_STRCT *p = descp->next_desc_addr;	//pointer to 2nd descriptor of frame
		HCFASSERT(p, 0)
		// The 2nd descriptor contains (maybe) a SNAP header plus part or whole of the payload.
		//determine decapsulation sub-flag in RxFS
		i = *(wci_recordp)&descp->buf_addr[HFS_STAT] & ( HFS_STAT_MSG_TYPE | HFS_STAT_ERR );
		if ( i == HFS_STAT_TUNNEL ||
			 ( i == HFS_STAT_1042 && hcf_encap( (wci_bufp)&p->buf_addr[HCF_DASA_SIZE] ) != ENC_TUNNEL )) {
			// The 2nd descriptor contains a SNAP header plus part or whole of the payload.
			HCFASSERT( p->BUF_CNT == (p->buf_addr[5] + (p->buf_addr[4]<<8) + 2*6 + 2 - 8), p->BUF_CNT )
			// perform decapsulation
			HCFASSERT(p->BUF_SIZE >=8, p->BUF_SIZE)
			// move SA[2:5] in the second buffer to replace part of the SNAP header
			for ( i=3; i >= 0; i--) p->buf_addr[i+8] = p->buf_addr[i];
			// copy DA[0:5], SA[0:1] from first buffer to second buffer
			for ( i=0; i<8; i++) p->buf_addr[i] = descp->buf_addr[HFS_ADDR_DEST + i];
			// make first buffer shorter in count
			descp->BUF_CNT = HFS_ADDR_DEST;
		}
	}
#endif // HCF_ENC
	if ( descp == NULL ) ifbp->IFB_DmaPackets &= (hcf_16)~HREG_EV_RDMAD;  //;?could be integrated into get_frame_lst
	HCFLOGEXIT( HCF_TRACE_DMA_RX_GET )
	return descp;
} // hcf_dma_rx_get


void
hcf_dma_rx_put( IFBP ifbp, DESC_STRCT *descp )
{

	HCFLOGENTRY( HCF_TRACE_DMA_RX_PUT, 0xDA01 )
	HCFASSERT( ifbp->IFB_Magic == HCF_MAGIC, ifbp->IFB_Magic )
	HCFASSERT_INT

	put_frame_lst( ifbp, descp, DMA_RX );
#if HCF_ASSERT && (HCF_ENCAP) == HCF_ENC
	if ( descp->buf_addr ) {
		HCFASSERT( descp->BUF_SIZE == HCF_DMA_RX_BUF1_SIZE, descp->BUF_SIZE )
		HCFASSERT( descp->next_desc_addr, 0 ) // first descriptor should be followed by another descriptor
		// The second DB is for SNAP and payload purposes. It should be a minimum of 12 bytes in size.
		HCFASSERT( descp->next_desc_addr->BUF_SIZE >= 12, descp->next_desc_addr->BUF_SIZE )
	}
#endif // HCFASSERT / HCF_ENC
	HCFLOGEXIT( HCF_TRACE_DMA_RX_PUT )
} // hcf_dma_rx_put


DESC_STRCT*
hcf_dma_tx_get( IFBP ifbp )
{
DESC_STRCT *descp;	// pointer to start of FrameList

	descp = get_frame_lst( ifbp, DMA_TX );
	if ( descp && descp->buf_addr )  //!be aware of the missing curly bracket
											//skip decapsulation at confined descriptor
#if (HCF_ENCAP) == HCF_ENC
		if ( ( descp->BUF_CNT == HFS_TYPE )
#if (HCF_TYPE) & HCF_TYPE_CCX
			 || ( descp->BUF_CNT == HFS_DAT )
#endif // HCF_TYPE_CCX
		) { // perform decapsulation if needed
			descp->next_desc_addr->buf_phys_addr -= HCF_DASA_SIZE;
			descp->next_desc_addr->BUF_CNT 		 += HCF_DASA_SIZE;
		}
#endif // HCF_ENC
	if ( descp == NULL ) {  //;?could be integrated into get_frame_lst
		ifbp->IFB_DmaPackets &= (hcf_16)~HREG_EV_TDMAD;
	}
	HCFLOGEXIT( HCF_TRACE_DMA_TX_GET )
	return descp;
} // hcf_dma_tx_get


void
hcf_dma_tx_put( IFBP ifbp, DESC_STRCT *descp, hcf_16 tx_cntl )
{
DESC_STRCT	*p = descp->next_desc_addr;
int			i;

#if HCF_ASSERT
	int x = ifbp->IFB_FWIdentity.comp_id == COMP_ID_FW_AP ? tx_cntl & ~HFS_TX_CNTL_PORT : tx_cntl;
	HCFASSERT( (x & ~HCF_TX_CNTL_MASK ) == 0, tx_cntl )
#endif // HCF_ASSERT
	HCFLOGENTRY( HCF_TRACE_DMA_TX_PUT, 0xDA03 )
	HCFASSERT( ifbp->IFB_Magic == HCF_MAGIC, ifbp->IFB_Magic )
	HCFASSERT_INT
	HCFASSERT( ( ifbp->IFB_CntlOpt & (USE_DMA|DMA_ENABLED) ) == (USE_DMA|DMA_ENABLED), ifbp->IFB_CntlOpt)

	if ( descp->buf_addr ) {
		*(hcf_16*)(descp->buf_addr + HFS_TX_CNTL) = tx_cntl;											/*1*/
#if (HCF_ENCAP) == HCF_ENC
		HCFASSERT( descp->next_desc_addr, 0 )									//at least 2 descripors
		HCFASSERT( descp->BUF_CNT == HFS_ADDR_DEST, descp->BUF_CNT )	//exact length required for 1st buffer
		HCFASSERT( descp->BUF_SIZE >= HCF_DMA_TX_BUF1_SIZE, descp->BUF_SIZE )	//minimal storage for encapsulation
		HCFASSERT( p->BUF_CNT >= 14, p->BUF_CNT );					//at least DA, SA and 'type' in 2nd buffer

#if (HCF_TYPE) & HCF_TYPE_CCX
		/* if we are doing PPK +/- CMIC, or we are sending a DDP frame */
		if ( ( ifbp->IFB_CKIPStat == HCF_ACT_CCX_ON ) ||
			 ( ( p->BUF_CNT >= 20 )		 && ( ifbp->IFB_CKIPStat == HCF_ACT_CCX_OFF ) &&
			 ( p->buf_addr[12] == 0xAA ) && ( p->buf_addr[13] == 0xAA ) &&
			 ( p->buf_addr[14] == 0x03 ) && ( p->buf_addr[15] == 0x00 ) &&
			 ( p->buf_addr[16] == 0x40 ) && ( p->buf_addr[17] == 0x96 ) &&
			 ( p->buf_addr[18] == 0x00 ) && ( p->buf_addr[19] == 0x00 )))
		{
			/* copy the DA/SA to the first buffer */
			for ( i = 0; i < HCF_DASA_SIZE; i++ ) {
				descp->buf_addr[i + HFS_ADDR_DEST] = p->buf_addr[i];
			}
			/* calculate the length of the second fragment only */
			i = 0;
			do { i += p->BUF_CNT; } while( p = p->next_desc_addr );
			i -= HCF_DASA_SIZE ;
			/* convert the length field to big endian, using the endian friendly macros */
			i = CNV_SHORT_TO_BIG(i);		//!! this converts ONLY on LE platforms, how does that relate to the non-CCX code
			*(hcf_16*)(&descp->buf_addr[HFS_LEN]) = (hcf_16)i;
			descp->BUF_CNT = HFS_DAT;
			// modify 2nd descriptor to skip the 'Da/Sa' fields
			descp->next_desc_addr->buf_phys_addr += HCF_DASA_SIZE;
			descp->next_desc_addr->BUF_CNT		 -= HCF_DASA_SIZE;
		}
		else
#endif // HCF_TYPE_CCX
		{
			descp->buf_addr[HFS_TYPE-1] = hcf_encap(&descp->next_desc_addr->buf_addr[HCF_DASA_SIZE]);		/*4*/
			if ( descp->buf_addr[HFS_TYPE-1] != ENC_NONE ) {
				for ( i=0; i < HCF_DASA_SIZE; i++ ) {														/*6*/
					descp->buf_addr[i + HFS_ADDR_DEST] = descp->next_desc_addr->buf_addr[i];
				}
				i = sizeof(snap_header) + 2 - ( 2*6 + 2 );
				do { i += p->BUF_CNT; } while ( ( p = p->next_desc_addr ) != NULL );
				*(hcf_16*)(&descp->buf_addr[HFS_LEN]) = CNV_END_SHORT(i);	//!! this converts on ALL platforms, how does that relate to the CCX code
				for ( i=0; i < sizeof(snap_header) - 1; i++) {
					descp->buf_addr[HFS_TYPE - sizeof(snap_header) + i] = snap_header[i];
				}
				descp->BUF_CNT = HFS_TYPE;																	/*8*/
				descp->next_desc_addr->buf_phys_addr	+= HCF_DASA_SIZE;
				descp->next_desc_addr->BUF_CNT			-= HCF_DASA_SIZE;
			}
		}
#endif // HCF_ENC
    }
	put_frame_lst( ifbp, descp, DMA_TX );
	HCFLOGEXIT( HCF_TRACE_DMA_TX_PUT )
} // hcf_dma_tx_put

#endif // HCF_DMA

#if (HCF_DL_ONLY) == 0
#if HCF_ENCAP	//i.e HCF_ENC or HCF_ENC_SUP
#if ! ( (HCF_ENCAP) & HCF_ENC_SUP )
HCF_STATIC
#endif // HCF_ENCAP
hcf_8
hcf_encap( wci_bufp type )
{

hcf_8	rc = ENC_NONE;																					/* 1 */
hcf_16	t = (hcf_16)(*type<<8) + *(type+1);																/* 2 */

	if ( t > 1500 ) {																					/* 4 */
		if ( t == 0x8137 || t == 0x80F3 ) {
			rc = ENC_TUNNEL;																			/* 6 */
		} else {
			rc = ENC_1042;
		}
	}
	return rc;
} // hcf_encap
#endif // HCF_ENCAP
#endif // HCF_DL_ONLY


int
hcf_get_info( IFBP ifbp, LTVP ltvp )
{

int			rc = HCF_SUCCESS;
hcf_16		len = ltvp->len;
hcf_16		type = ltvp->typ;
wci_recordp	p = &ltvp->len;		//destination word pointer (in LTV record)
hcf_16		*q = NULL;				/* source word pointer  Note!! DOS COM can't cope with FAR
									 * as a consequence MailBox must be near which is usually true anyway
									 */
int			i;

	HCFLOGENTRY( HCF_TRACE_GET_INFO, ltvp->typ )
	HCFASSERT( ifbp->IFB_Magic == HCF_MAGIC, ifbp->IFB_Magic )
	HCFASSERT_INT
	HCFASSERT( ltvp, 0 )
	HCFASSERT( 1 < ltvp->len && ltvp->len <= HCF_MAX_LTV + 1, MERGE_2( ltvp->typ, ltvp->len ) )

	ltvp->len = 0;								//default to: No Info Available
#if defined MSF_COMPONENT_ID || (HCF_EXT) & HCF_EXT_MB //filter out all specials
	for ( i = 0; ( q = xxxx[i] ) != NULL && q[1] != type; i++ ) /*NOP*/;
#endif // MSF_COMPONENT_ID / HCF_EXT_MB
#if HCF_TALLIES
	if ( type == CFG_TALLIES ) {													/*3*/
		(void)hcf_action( ifbp, HCF_ACT_TALLIES );
		q = (hcf_16*)&ifbp->IFB_TallyLen;
	}
#endif // HCF_TALLIES
#if (HCF_EXT) & HCF_EXT_MB
	if ( type == CFG_MB_INFO ) {
		if ( ifbp->IFB_MBInfoLen ) {
			if ( ifbp->IFB_MBp[ifbp->IFB_MBRp] == 0xFFFF ) {
				ifbp->IFB_MBRp = 0; //;?Probably superfluous
			}
			q = &ifbp->IFB_MBp[ifbp->IFB_MBRp];
			ifbp->IFB_MBRp += *q + 1;	//update read pointer
			if ( ifbp->IFB_MBp[ifbp->IFB_MBRp] == 0xFFFF ) {
				ifbp->IFB_MBRp = 0;
			}
			ifbp->IFB_MBInfoLen = ifbp->IFB_MBp[ifbp->IFB_MBRp];
		}
	}
#endif // HCF_EXT_MB
	if ( q != NULL ) {						//a special or CFG_TALLIES or CFG_MB_INFO
		i = min( len, *q ) + 1;				//total size of destination (including T-field)
		while ( i-- ) {
			*p++ = *q;
#if (HCF_TALLIES) & HCF_TALLIES_RESET
			if ( q > &ifbp->IFB_TallyTyp && type == CFG_TALLIES ) {
				*q = 0;
			}
#endif // HCF_TALLIES_RESET
			q++;
		}
	} else {								// not a special nor CFG_TALLIES nor CFG_MB_INFO
		if ( type == CFG_CNTL_OPT ) {										//read back effective options
			ltvp->len = 2;
			ltvp->val[0] = ifbp->IFB_CntlOpt;
#if (HCF_EXT) & HCF_EXT_NIC_ACCESS
		} else if ( type == CFG_PROD_DATA ) {  //only needed for some test tool on top of H-II NDIS driver
hcf_io		io_port;
wci_bufp	pt;					//pointer with the "right" type, just to help ease writing macros with embedded assembly
			OPW( HREG_AUX_PAGE, (hcf_16)(PLUG_DATA_OFFSET >> 7) );
			OPW( HREG_AUX_OFFSET, (hcf_16)(PLUG_DATA_OFFSET & 0x7E) );
			io_port = ifbp->IFB_IOBase + HREG_AUX_DATA;		//to prevent side effects of the MSF-defined macro
			p = ltvp->val;					//destination char pointer (in LTV record)
			i = len - 1;
			if (i > 0 ) {
				pt = (wci_bufp)p;	//just to help ease writing macros with embedded assembly
				IN_PORT_STRING_8_16( io_port, pt, i ); //space used by T: -1
			}
		} else if ( type == CFG_CMD_HCF ) {
#define P ((CFG_CMD_HCF_STRCT FAR *)ltvp)
			HCFASSERT( P->cmd == CFG_CMD_HCF_REG_ACCESS, P->cmd )		//only Hermes register access supported
			if ( P->cmd == CFG_CMD_HCF_REG_ACCESS ) {
				HCFASSERT( P->mode < ifbp->IFB_IOBase, P->mode )		//Check Register space
				ltvp->len = min( len, 4 );								//RESTORE ltv length
				P->add_info = IPW( P->mode );
			}
#undef P
#endif // HCF_EXT_NIC_ACCESS
#if (HCF_ASSERT) & HCF_ASSERT_PRINTF
        } else if (type == CFG_FW_PRINTF) {
           rc = fw_printf(ifbp, (CFG_FW_PRINTF_STRCT*)ltvp);
#endif // HCF_ASSERT_PRINTF
		} else if ( type >= CFG_RID_FW_MIN ) {
//;? by using HCMD_BUSY option when calling cmd_exe, using a get_frag with length 0 just to set up the
//;? BAP and calling cmd_cmpl, you could merge the 2 Busy waits. Whether this really helps (and what
//;? would be the optimal sequence in cmd_exe and get_frag) would have to be MEASURED
/*17*/		if ( ( rc = cmd_exe( ifbp, HCMD_ACCESS, type ) ) == HCF_SUCCESS &&
				 ( rc = setup_bap( ifbp, type, 0, IO_IN ) ) == HCF_SUCCESS ) {
				get_frag( ifbp, (wci_bufp)&ltvp->len, 2*len+2 BE_PAR(2) );
				if ( IPW( HREG_STAT ) == 0xFFFF ) {					//NIC removal test
					ltvp->len = 0;
					HCFASSERT( DO_ASSERT, type )
				}
			}
/*12*/	} else HCFASSERT( DO_ASSERT, type ) /*NOP*/; //NOP in case HCFASSERT is dummy
	}
	if ( len < ltvp->len ) {
		ltvp->len = len;
		if ( rc == HCF_SUCCESS ) {
			rc = HCF_ERR_LEN;
		}
	}
	HCFASSERT( rc == HCF_SUCCESS || ( rc == HCF_ERR_LEN && ifbp->IFB_AssertTrace & 1<<HCF_TRACE_PUT_INFO ),
			   MERGE_2( type, rc ) )																/*20*/
	HCFLOGEXIT( HCF_TRACE_GET_INFO )
	return rc;
} // hcf_get_info



int
hcf_put_info( IFBP ifbp, LTVP ltvp )
{
int rc = HCF_SUCCESS;

	HCFLOGENTRY( HCF_TRACE_PUT_INFO, ltvp->typ )
	HCFASSERT( ifbp->IFB_Magic == HCF_MAGIC, ifbp->IFB_Magic )
	HCFASSERT_INT
	HCFASSERT( ltvp, 0 )
	HCFASSERT( 1 < ltvp->len && ltvp->len <= HCF_MAX_LTV + 1, ltvp->len )

											//all codes between 0xFA00 and 0xFCFF are passed to Hermes
#if (HCF_TYPE) & HCF_TYPE_WPA
 {	hcf_16 i;
	hcf_32 FAR * key_p;

	if ( ltvp->typ == CFG_ADD_TKIP_DEFAULT_KEY || ltvp->typ == CFG_ADD_TKIP_MAPPED_KEY ) {
		key_p = (hcf_32*)((CFG_ADD_TKIP_MAPPED_KEY_STRCT FAR *)ltvp)->tx_mic_key;
		i = TX_KEY;		//i.e. TxKeyIndicator == 1, KeyID == 0
		if ( ltvp->typ == CFG_ADD_TKIP_DEFAULT_KEY ) {
			key_p = (hcf_32*)((CFG_ADD_TKIP_DEFAULT_KEY_STRCT FAR *)ltvp)->tx_mic_key;
			i = CNV_LITTLE_TO_SHORT(((CFG_ADD_TKIP_DEFAULT_KEY_STRCT FAR *)ltvp)->tkip_key_id_info);
		}
		if ( i & TX_KEY ) {	/* TxKeyIndicator == 1
							   (either really set by MSF in case of DEFAULT or faked by HCF in case of MAPPED ) */
			ifbp->IFB_MICTxCntl = (hcf_16)( HFS_TX_CNTL_MIC | (i & KEY_ID )<<8 );
			ifbp->IFB_MICTxKey[0] = CNV_LONGP_TO_LITTLE( key_p );
			ifbp->IFB_MICTxKey[1] = CNV_LONGP_TO_LITTLE( (key_p+1) );
		}
		i = ( i & KEY_ID ) * 2;
		ifbp->IFB_MICRxKey[i]   = CNV_LONGP_TO_LITTLE( (key_p+2) );
		ifbp->IFB_MICRxKey[i+1] = CNV_LONGP_TO_LITTLE( (key_p+3) );
	}
#define P ((CFG_REMOVE_TKIP_DEFAULT_KEY_STRCT FAR *)ltvp)
	if ( ( ltvp->typ == CFG_REMOVE_TKIP_MAPPED_KEY )	||
		 ( ltvp->typ == CFG_REMOVE_TKIP_DEFAULT_KEY &&
		   ( (ifbp->IFB_MICTxCntl >> 8) & KEY_ID ) == CNV_SHORT_TO_LITTLE(P->tkip_key_id )
		 )
		) { ifbp->IFB_MICTxCntl = 0; }		//disable MIC-engine
#undef P
 }
#endif // HCF_TYPE_WPA

	if ( ltvp->typ == CFG_PROG ) {
		rc = download( ifbp, (CFG_PROG_STRCT FAR *)ltvp );
	} else switch (ltvp->typ) {
#if (HCF_ASSERT) & HCF_ASSERT_RT_MSF_RTN
	  case CFG_REG_ASSERT_RTNP:											//Register MSF Routines
#define P ((CFG_REG_ASSERT_RTNP_STRCT FAR *)ltvp)
		ifbp->IFB_AssertRtn = P->rtnp;
//		ifbp->IFB_AssertLvl = P->lvl;		//TODO not yet supported so default is set in hcf_connect
		HCFASSERT( DO_ASSERT, MERGE_2( HCF_ASSERT, 0xCAF1 ) )	//just to proof that the complete assert machinery is working
#undef P
		break;
#endif // HCF_ASSERT_RT_MSF_RTN
#if (HCF_EXT) & HCF_EXT_INFO_LOG
	  case CFG_REG_INFO_LOG:											//Register Log filter
		ifbp->IFB_RIDLogp = ((CFG_RID_LOG_STRCT FAR*)ltvp)->recordp;
		break;
#endif // HCF_EXT_INFO_LOG
	  case CFG_CNTL_OPT:												//overrule option
		HCFASSERT( ( ltvp->val[0] & ~(USE_DMA | USE_16BIT) ) == 0, ltvp->val[0] )
		if ( ( ltvp->val[0] & USE_DMA ) == 0 ) ifbp->IFB_CntlOpt &= ~USE_DMA;
		ifbp->IFB_CntlOpt |=  ltvp->val[0] & USE_16BIT;
		break;
#if (HCF_EXT) & HCF_EXT_MB
	  case CFG_REG_MB:													//Register MailBox
#define P ((CFG_REG_MB_STRCT FAR *)ltvp)
		HCFASSERT( ( (hcf_32)P->mb_addr & 0x0001 ) == 0, (hcf_32)P->mb_addr )
		HCFASSERT( (P)->mb_size >= 60, (P)->mb_size )
		ifbp->IFB_MBp = P->mb_addr;
		/* if no MB present, size must be 0 for ;?the old;? put_info_mb to work correctly */
		ifbp->IFB_MBSize = ifbp->IFB_MBp == NULL ? 0 : P->mb_size;
		ifbp->IFB_MBWp = ifbp->IFB_MBRp	= 0;
		ifbp->IFB_MBp[0] = 0;											//flag the MailBox as empty
		ifbp->IFB_MBInfoLen = 0;
		HCFASSERT( ifbp->IFB_MBSize >= 60 || ifbp->IFB_MBp == NULL, ifbp->IFB_MBSize )
#undef P
		break;
	  case CFG_MB_INFO:													//store MailBoxInfoBlock
		rc = put_info_mb( ifbp, (CFG_MB_INFO_STRCT FAR *)ltvp );
		break;
#endif // HCF_EXT_MB

#if (HCF_EXT) & HCF_EXT_NIC_ACCESS
	  case CFG_CMD_NIC:
#define P ((CFG_CMD_NIC_STRCT FAR *)ltvp)
		OPW( HREG_PARAM_2, P->parm2 );
		OPW( HREG_PARAM_1, P->parm1 );
		rc = cmd_exe( ifbp, P->cmd, P->parm0 );
		P->hcf_stat = (hcf_16)rc;
		P->stat = IPW( HREG_STAT );
		P->resp0 = IPW( HREG_RESP_0 );
		P->resp1 = IPW( HREG_RESP_1 );
		P->resp2 = IPW( HREG_RESP_2 );
		P->ifb_err_cmd = ifbp->IFB_ErrCmd;
		P->ifb_err_qualifier = ifbp->IFB_ErrQualifier;
#undef P
		break;
	  case CFG_CMD_HCF:
#define P ((CFG_CMD_HCF_STRCT FAR *)ltvp)
		HCFASSERT( P->cmd == CFG_CMD_HCF_REG_ACCESS, P->cmd )		//only Hermes register access supported
		if ( P->cmd == CFG_CMD_HCF_REG_ACCESS ) {
			HCFASSERT( P->mode < ifbp->IFB_IOBase, P->mode )		//Check Register space
			OPW( P->mode, P->add_info);
		}
#undef P
		break;
#endif // HCF_EXT_NIC_ACCESS

#if (HCF_ASSERT) & HCF_ASSERT_PRINTF
      case CFG_FW_PRINTF_BUFFER_LOCATION:
        ifbp->IFB_FwPfBuff = *(CFG_FW_PRINTF_BUFFER_LOCATION_STRCT*)ltvp;
        break;
#endif // HCF_ASSERT_PRINTF

	  default:						//pass everything unknown above the "FID" range to the Hermes or Dongle
		rc = put_info( ifbp, ltvp );
	}
	//DO NOT !!! HCFASSERT( rc == HCF_SUCCESS, rc )												/* 20 */
	HCFLOGEXIT( HCF_TRACE_PUT_INFO )
	return rc;
} // hcf_put_info


#if (HCF_DL_ONLY) == 0
int
hcf_rcv_msg( IFBP ifbp, DESC_STRCT *descp, unsigned int offset )
{
int			rc = HCF_SUCCESS;
wci_bufp	cp;										//char oriented working pointer
hcf_16		i;
int			tot_len = ifbp->IFB_RxLen - offset;		//total length
wci_bufp	lap = ifbp->IFB_lap + offset;			//start address in LookAhead Buffer
hcf_16		lal = ifbp->IFB_lal - offset;			//available data within LookAhead Buffer
hcf_16		j;

	HCFLOGENTRY( HCF_TRACE_RCV_MSG, offset )
	HCFASSERT( ifbp->IFB_Magic == HCF_MAGIC, ifbp->IFB_Magic )
	HCFASSERT_INT
	HCFASSERT( descp, HCF_TRACE_RCV_MSG )
	HCFASSERT( ifbp->IFB_RxLen, HCF_TRACE_RCV_MSG )
	HCFASSERT( ifbp->IFB_RxLen >= offset, MERGE_2( offset, ifbp->IFB_RxLen ) )
	HCFASSERT( ifbp->IFB_lal >= offset, offset )
	HCFASSERT( (ifbp->IFB_CntlOpt & USE_DMA) == 0, 0xDADA )

	if ( tot_len < 0 ) {
		lal = 0; tot_len = 0;				//suppress all copying activity in the do--while loop
	}
	do {									//loop over all available fragments
		// obnoxious hcf.c(1480) : warning C4769: conversion of near pointer to long integer
		HCFASSERT( ((hcf_32)descp & 3 ) == 0, (hcf_32)descp )
		cp = descp->buf_addr;
		j = min( (hcf_16)tot_len, descp->BUF_SIZE );	//minimum of "what's` available" and fragment size
		descp->BUF_CNT = j;
		tot_len -= j;						//adjust length still to go
		if ( lal ) {						//if lookahead Buffer not yet completely copied
			i = min( lal, j );				//minimum of "what's available" in LookAhead and fragment size
			lal -= i;						//adjust length still available in LookAhead
			j -= i;							//adjust length still available in current fragment
			/*;? while loop could be improved by moving words but that is complicated on platforms with
			 * alignment requirements*/
			while ( i-- ) *cp++ = *lap++;
		}
		if ( j ) {	//if LookAhead Buffer exhausted but still space in fragment, copy directly from NIC RAM
			get_frag( ifbp, cp, j BE_PAR(0) );
			CALC_RX_MIC( cp, j );
		}
	} while ( ( descp = descp->next_desc_addr ) != NULL );
#if (HCF_TYPE) & HCF_TYPE_WPA
	if ( ifbp->IFB_RxFID ) {
		rc = check_mic( ifbp );				//prevents MIC error report if hcf_service_nic already consumed all
	}
#endif // HCF_TYPE_WPA
	(void)hcf_action( ifbp, HCF_ACT_RX_ACK );		//only 1 shot to get the data, so free the resources in the NIC
	HCFASSERT( rc == HCF_SUCCESS, rc )
	HCFLOGEXIT( HCF_TRACE_RCV_MSG )
	return rc;
} // hcf_rcv_msg
#endif // HCF_DL_ONLY


#if (HCF_DL_ONLY) == 0
int
hcf_send_msg( IFBP ifbp, DESC_STRCT *descp, hcf_16 tx_cntl )
{
int			rc = HCF_SUCCESS;
DESC_STRCT	*p /* = descp*/;		//working pointer
hcf_16		len;					// total byte count
hcf_16		i;

hcf_16		fid = 0;

	HCFASSERT( ifbp->IFB_RscInd || descp == NULL, ifbp->IFB_RscInd )
	HCFASSERT( (ifbp->IFB_CntlOpt & USE_DMA) == 0, 0xDADB )

	HCFLOGENTRY( HCF_TRACE_SEND_MSG, tx_cntl )
	HCFASSERT( ifbp->IFB_Magic == HCF_MAGIC, ifbp->IFB_Magic )
	HCFASSERT_INT
	/* obnoxious c:/hcf/hcf.c(1480) : warning C4769: conversion of near pointer to long integer,
	 * so skip */
	HCFASSERT( ((hcf_32)descp & 3 ) == 0, (hcf_32)descp )
#if HCF_ASSERT
{	int x = ifbp->IFB_FWIdentity.comp_id == COMP_ID_FW_AP ? tx_cntl & ~HFS_TX_CNTL_PORT : tx_cntl;
	HCFASSERT( (x & ~HCF_TX_CNTL_MASK ) == 0, tx_cntl )
}
#endif // HCF_ASSERT

	if ( descp ) ifbp->IFB_TxFID = 0;				//cancel a pre-put message

#if (HCF_EXT) & HCF_EXT_TX_CONT				// Continuous transmit test
	if ( tx_cntl == HFS_TX_CNTL_TX_CONT ) {
	 	fid = get_fid(ifbp);
	 	if (fid != 0 ) {
											//setup BAP to begin of TxFS
			(void)setup_bap( ifbp, fid, 0, IO_OUT );
											//copy all the fragments in a transparent fashion
		 	for ( p = descp; p; p = p->next_desc_addr ) {
			/* obnoxious warning C4769: conversion of near pointer to long integer */
				HCFASSERT( ((hcf_32)p & 3 ) == 0, (hcf_32)p )
				put_frag( ifbp, p->buf_addr, p->BUF_CNT BE_PAR(0) );
			}
			rc = cmd_exe( ifbp, HCMD_THESEUS | HCMD_BUSY | HCMD_STARTPREAMBLE, fid );
			if ( ifbp->IFB_RscInd == 0 ) {
				ifbp->IFB_RscInd = get_fid( ifbp );
			}
		}
											// een slecht voorbeeld doet goed volgen ;?
		HCFLOGEXIT( HCF_TRACE_SEND_MSG )
		return rc;
	}
#endif // HCF_EXT_TX_CONT
									/* the following initialization code is redundant for a pre-put message
									 * but moving it inside the "if fid" logic makes the merging with the
									 * USB flow awkward
									 */
#if (HCF_TYPE) & HCF_TYPE_WPA
	tx_cntl |= ifbp->IFB_MICTxCntl;
#endif // HCF_TYPE_WPA
	fid = ifbp->IFB_TxFID;
	if (fid == 0 && ( fid = get_fid( ifbp ) ) != 0 ) 		/* 4 */
			/* skip the next compound statement if:
			   - pre-put message or
			   - no fid available (which should never occur if the MSF adheres to the WCI)
			 */
	{		// to match the closing curly bracket of above "if" in case of HCF_TYPE_USB
											//calculate total length ;? superfluous unless CCX or Encapsulation
		len = 0;
		p = descp;
		do len += p->BUF_CNT; while ( ( p = p->next_desc_addr ) != NULL );
		p = descp;
//;?	HCFASSERT( len <= HCF_MAX_MSG, len )
/*7*/	(void)setup_bap( ifbp, fid, HFS_TX_CNTL, IO_OUT );
#if (HCF_TYPE) & HCF_TYPE_TX_DELAY
		HCFASSERT( ( descp != NULL ) ^ ( tx_cntl & HFS_TX_CNTL_TX_DELAY ), tx_cntl )
		if ( tx_cntl & HFS_TX_CNTL_TX_DELAY ) {
			tx_cntl &= ~HFS_TX_CNTL_TX_DELAY;		//!!HFS_TX_CNTL_TX_DELAY no longer available
			ifbp->IFB_TxFID = fid;
			fid = 0;								//!!fid no longer available, be careful when modifying code
		}
#endif // HCF_TYPE_TX_DELAY
		OPW( HREG_DATA_1, tx_cntl ) ;
		OPW( HREG_DATA_1, 0 );
#if ! ( (HCF_TYPE) & HCF_TYPE_CCX )
		HCFASSERT( p->BUF_CNT >= 14, p->BUF_CNT )
											/* assume DestAddr/SrcAddr/Len/Type ALWAYS contained in 1st fragment
											 * otherwise life gets too cumbersome for MIC and Encapsulation !!!!!!!!
		if ( p->BUF_CNT >= 14 ) {	alternatively: add a safety escape !!!!!!!!!!!! }	*/
#endif // HCF_TYPE_CCX
		CALC_TX_MIC( NULL, -1 );		//initialize MIC
/*10*/	put_frag( ifbp, p->buf_addr, HCF_DASA_SIZE BE_PAR(0) );	//write DA, SA with MIC calculation
		CALC_TX_MIC( p->buf_addr, HCF_DASA_SIZE );		//MIC over DA, SA
		CALC_TX_MIC( null_addr, 4 );		//MIC over (virtual) priority field
#if (HCF_TYPE) & HCF_TYPE_CCX
		//!!be careful do not use positive test on HCF_ACT_CCX_OFF, because IFB_CKIPStat is initially 0
		if(( ifbp->IFB_CKIPStat == HCF_ACT_CCX_ON ) ||
           ((GET_BUF_CNT(p) >= 20 )   && ( ifbp->IFB_CKIPStat == HCF_ACT_CCX_OFF ) &&
            (p->buf_addr[12] == 0xAA) && (p->buf_addr[13] == 0xAA) &&
            (p->buf_addr[14] == 0x03) && (p->buf_addr[15] == 0x00) &&
            (p->buf_addr[16] == 0x40) && (p->buf_addr[17] == 0x96) &&
            (p->buf_addr[18] == 0x00) && (p->buf_addr[19] == 0x00)))
        {
            i = HCF_DASA_SIZE;

            OPW( HREG_DATA_1, CNV_SHORT_TO_BIG( len - i ));

            /* need to send out the remainder of the fragment */
			put_frag( ifbp, &p->buf_addr[i], GET_BUF_CNT(p) - i BE_PAR(0) );
        }
        else
#endif // HCF_TYPE_CCX
		{
											//if encapsulation needed
#if (HCF_ENCAP) == HCF_ENC
											//write length (with SNAP-header,Type, without //DA,SA,Length ) no MIC calc.
			if ( ( snap_header[sizeof(snap_header)-1] = hcf_encap( &p->buf_addr[HCF_DASA_SIZE] ) ) != ENC_NONE ) {
				OPW( HREG_DATA_1, CNV_END_SHORT( len + (sizeof(snap_header) + 2) - ( 2*6 + 2 ) ) );
											//write splice with MIC calculation
				put_frag( ifbp, snap_header, sizeof(snap_header) BE_PAR(0) );
				CALC_TX_MIC( snap_header, sizeof(snap_header) );	//MIC over 6 byte SNAP
				i = HCF_DASA_SIZE;
			} else
#endif // HCF_ENC
			{
				OPW( HREG_DATA_1, *(wci_recordp)&p->buf_addr[HCF_DASA_SIZE] );
				i = 14;
			}
											//complete 1st fragment starting with Type with MIC calculation
			put_frag( ifbp, &p->buf_addr[i], p->BUF_CNT - i BE_PAR(0) );
			CALC_TX_MIC( &p->buf_addr[i], p->BUF_CNT - i );
		}
											//do the remaining fragments with MIC calculation
		while ( ( p = p->next_desc_addr ) != NULL ) {
			/* obnoxious c:/hcf/hcf.c(1480) : warning C4769: conversion of near pointer to long integer,
			 * so skip */
			HCFASSERT( ((hcf_32)p & 3 ) == 0, (hcf_32)p )
			put_frag( ifbp, p->buf_addr, p->BUF_CNT BE_PAR(0) );
			CALC_TX_MIC( p->buf_addr, p->BUF_CNT );
		}
											//pad message, finalize MIC calculation and write MIC to NIC
		put_frag_finalize( ifbp );
	}
	if ( fid ) {
/*16*/	rc = cmd_exe( ifbp, HCMD_BUSY | HCMD_TX | HCMD_RECL, fid );
		ifbp->IFB_TxFID = 0;
			/* probably this (i.e. no RscInd AND "HREG_EV_ALLOC") at this point in time occurs so infrequent,
			 * that it might just as well be acceptable to skip this
			 * "optimization" code and handle that additional interrupt once in a while
			 */
// 180 degree error in logic ;? #if ALLOC_15
/*20*/	if ( ifbp->IFB_RscInd == 0 ) {
			ifbp->IFB_RscInd = get_fid( ifbp );
		}
// #endif // ALLOC_15
	}
//	HCFASSERT( level::ifbp->IFB_RscInd, ifbp->IFB_RscInd )
	HCFLOGEXIT( HCF_TRACE_SEND_MSG )
	return rc;
} // hcf_send_msg
#endif // HCF_DL_ONLY


#if (HCF_DL_ONLY) == 0
int
hcf_service_nic( IFBP ifbp, wci_bufp bufp, unsigned int len )
{

int			rc = HCF_SUCCESS;
hcf_16		stat;
wci_bufp	buf_addr;
hcf_16 		i;

	HCFLOGENTRY( HCF_TRACE_SERVICE_NIC, ifbp->IFB_IntOffCnt )
	HCFASSERT( ifbp->IFB_Magic == HCF_MAGIC, ifbp->IFB_Magic )
	HCFASSERT_INT

	ifbp->IFB_LinkStat = 0; // ;? to be obsoleted ASAP												/* 1*/
	ifbp->IFB_DSLinkStat &= ~CFG_LINK_STAT_CHANGE;													/* 1*/
	(void)hcf_action( ifbp, HCF_ACT_RX_ACK );														/* 2*/
	if ( ifbp->IFB_CardStat == 0 && ( stat = IPW( HREG_EV_STAT ) ) != 0xFFFF ) {					/* 4*/
																									/* 8*/
		if ( ifbp->IFB_RscInd == 0 && stat & HREG_EV_ALLOC ) { //Note: IFB_RscInd is ALWAYS 1 for DMA
			ifbp->IFB_RscInd = 1;
		}
		IF_TALLY( if ( stat & HREG_EV_INFO_DROP ) ifbp->IFB_HCF_Tallies.NoBufInfo++; )
#if (HCF_EXT) & HCF_EXT_INT_TICK
		if ( stat & HREG_EV_TICK ) {
			ifbp->IFB_TickCnt++;
		}
#if 0 // (HCF_SLEEP) & HCF_DDS
		if ( ifbp->IFB_TickCnt == 3 && ( ifbp->IFB_DSLinkStat & CFG_LINK_STAT_CONNECTED ) == 0 ) {
CFG_DDS_TICK_TIME_STRCT ltv;
			// 2 second period (with 1 tick uncertanty) in not-connected mode -->go into DS_OOR
			hcf_action( ifbp, HCF_ACT_SLEEP );
			ifbp->IFB_DSLinkStat |= CFG_LINK_STAT_DS_OOR; //set OutOfRange
			ltv.len = 2;
			ltv.typ = CFG_DDS_TICK_TIME;
			ltv.tick_time = ( ( ifbp->IFB_DSLinkStat & CFG_LINK_STAT_TIMER ) + 0x10 ) *64; //78 is more right
			hcf_put_info( ifbp, (LTVP)&ltv );
			printk( "<5>Preparing for sleep, link_status: %04X, timer : %d\n",
					ifbp->IFB_DSLinkStat, ltv.tick_time );//;?remove me 1 day
			ifbp->IFB_TickCnt++; //;?just to make sure we do not keep on printing above message
			if ( ltv.tick_time < 300 * 125 ) ifbp->IFB_DSLinkStat += 0x0010;

		}
#endif // HCF_DDS
#endif // HCF_EXT_INT_TICK
		if ( stat & HREG_EV_INFO ) {
			isr_info( ifbp );
		}
#if (HCF_EXT) & HCF_EXT_INT_TX_EX
		if ( stat & HREG_EV_TX_EXT && ( i = IPW( HREG_TX_COMPL_FID ) ) != 0 /*DAWA*/ ) {
			DAWA_ZERO_FID( HREG_TX_COMPL_FID )
			(void)setup_bap( ifbp, i, 0, IO_IN );
			get_frag( ifbp, &ifbp->IFB_TxFsStat, HFS_SWSUP BE_PAR(1) );
		}
#endif // HCF_EXT_INT_TX_EX
//!rlav DMA engine will handle the rx event, not the driver
#if HCF_DMA
		if ( !( ifbp->IFB_CntlOpt & USE_DMA ) ) //!! be aware of the logical indentations
#endif // HCF_DMA
/*16*/	  if ( stat & HREG_EV_RX && ( ifbp->IFB_RxFID = IPW( HREG_RX_FID ) ) != 0 ) { //if 0 then DAWA_ACK
			HCFASSERT( bufp, len )
			HCFASSERT( len >= HFS_DAT + 2, len )
			DAWA_ZERO_FID( HREG_RX_FID )
			HCFASSERT( ifbp->IFB_RxFID < CFG_PROD_DATA, ifbp->IFB_RxFID)
			(void)setup_bap( ifbp, ifbp->IFB_RxFID, 0, IO_IN );
			get_frag( ifbp, bufp, HFS_ADDR_DEST BE_PAR(1) );
			ifbp->IFB_lap = buf_addr = bufp + HFS_ADDR_DEST;
			ifbp->IFB_RxLen = (hcf_16)(bufp[HFS_DAT_LEN] + (bufp[HFS_DAT_LEN+1]<<8) + 2*6 + 2);
/*26*/		if ( ifbp->IFB_RxLen >= 22 ) {		// convenient for MIC calculation (5 DWs + 1 "skipped" W)
												//.  get DA,SA,Len/Type and (SNAP,Type or 8 data bytes)
/*30*/			get_frag( ifbp, buf_addr, 22 BE_PAR(0) );
/*32*/			CALC_RX_MIC( bufp, -1 );		//.  initialize MIC
				CALC_RX_MIC( buf_addr, HCF_DASA_SIZE );	//.  MIC over DA, SA
				CALC_RX_MIC( null_addr, 4 );	//.  MIC over (virtual) priority field
				CALC_RX_MIC( buf_addr+14, 8 );	//.  skip Len, MIC over SNAP,Type or 8 data bytes)
				buf_addr += 22;
#if (HCF_TYPE) & HCF_TYPE_CCX
//!!be careful do not use positive test on HCF_ACT_CCX_OFF, because IFB_CKIPStat is initially 0
				if( ifbp->IFB_CKIPStat != HCF_ACT_CCX_ON  )
#endif // HCF_TYPE_CCX
				{
#if (HCF_ENCAP) == HCF_ENC
					HCFASSERT( len >= HFS_DAT + 2 + sizeof(snap_header), len )
/*34*/ 				i = *(wci_recordp)&bufp[HFS_STAT] & ( HFS_STAT_MSG_TYPE | HFS_STAT_ERR );
					if ( i == HFS_STAT_TUNNEL ||
						 ( i == HFS_STAT_1042 && hcf_encap( (wci_bufp)&bufp[HFS_TYPE] ) != ENC_TUNNEL ) ) {
												//.  copy E-II Type to 802.3 LEN field
/*36*/					bufp[HFS_LEN  ] = bufp[HFS_TYPE  ];
						bufp[HFS_LEN+1] = bufp[HFS_TYPE+1];
												//.  discard Snap by overwriting with data
						ifbp->IFB_RxLen -= (HFS_TYPE - HFS_LEN);
						buf_addr -= ( HFS_TYPE - HFS_LEN ); // this happens to bring us at a DW boundary of 36
					}
#endif // HCF_ENC
				}
			}
/*40*/		ifbp->IFB_lal = min( (hcf_16)(len - HFS_ADDR_DEST), ifbp->IFB_RxLen );
			i = ifbp->IFB_lal - ( buf_addr - ( bufp + HFS_ADDR_DEST ) );
			get_frag( ifbp, buf_addr, i BE_PAR(0) );
			CALC_RX_MIC( buf_addr, i );
#if (HCF_TYPE) & HCF_TYPE_WPA
			if ( ifbp->IFB_lal == ifbp->IFB_RxLen ) {
				rc = check_mic( ifbp );
			}
#endif // HCF_TYPE_WPA
/*44*/		if ( len - HFS_ADDR_DEST >= ifbp->IFB_RxLen ) {
				ifbp->IFB_RxFID = 0;
			} else { /* IFB_RxFID is cleared, so  you do not get another Rx_Ack at next entry of hcf_service_nic */
				stat &= (hcf_16)~HREG_EV_RX;	//don't ack Rx if processing not yet completed
			}
		}
		// in case of DMA: signal availability of rx and/or tx packets to MSF
		IF_USE_DMA( ifbp->IFB_DmaPackets |= stat & ( HREG_EV_RDMAD | HREG_EV_TDMAD ); )
		// rlav : pending HREG_EV_RDMAD or HREG_EV_TDMAD events get acknowledged here.
/*54*/	stat &= (hcf_16)~( HREG_EV_SLEEP_REQ | HREG_EV_CMD | HREG_EV_ACK_REG_READY | HREG_EV_ALLOC | HREG_EV_FW_DMA );
//a positive mask would be easier to understand /*54*/	stat &= (hcf_16)~( HREG_EV_SLEEP_REQ | HREG_EV_CMD | HREG_EV_ACK_REG_READY | HREG_EV_ALLOC | HREG_EV_FW_DMA );
		IF_USE_DMA( stat &= (hcf_16)~HREG_EV_RX; )
		if ( stat ) {
			DAWA_ACK( stat );	/*DAWA*/
		}
	}
	HCFLOGEXIT( HCF_TRACE_SERVICE_NIC )
	return rc;
} // hcf_service_nic
#endif // HCF_DL_ONLY





#if (HCF_TYPE) & HCF_TYPE_WPA

#define ROL32( A, n ) ( ((A) << (n)) | ( ((A)>>(32-(n)))  & ( (1UL << (n)) - 1 ) ) )
#define ROR32( A, n ) ROL32( (A), 32-(n) )

#define L	*p
#define R	*(p+1)

void
calc_mic( hcf_32* p, hcf_32 m )
{
#if HCF_BIG_ENDIAN
	m = (m >> 16) | (m << 16);
#endif // HCF_BIG_ENDIAN
	L ^= m;
	R ^= ROL32( L, 17 );
	L += R;
	R ^= ((L & 0xff00ff00) >> 8) | ((L & 0x00ff00ff) << 8);
	L += R;
	R ^= ROL32( L, 3 );
	L += R;
	R ^= ROR32( L, 2 );
	L += R;
} // calc_mic
#undef R
#undef L
#endif // HCF_TYPE_WPA



#if (HCF_TYPE) & HCF_TYPE_WPA
void
calc_mic_rx_frag( IFBP ifbp, wci_bufp p, int len )
{
static union { hcf_32 x32; hcf_16 x16[2]; hcf_8 x8[4]; } x;	//* area to accumulate 4 bytes input for MIC engine
int i;

	if ( len == -1 ) {								//initialize MIC housekeeping
		i = *(wci_recordp)&p[HFS_STAT];
		/* i = CNV_SHORTP_TO_LITTLE(&p[HFS_STAT]); should not be neede to prevent alignment poroblems
		 * since len == -1 if and only if p is lookahaead buffer which MUST be word aligned
		 * to be re-investigated by NvR
		 */

		if ( ( i & HFS_STAT_MIC ) == 0 ) {
			ifbp->IFB_MICRxCarry = 0xFFFF;			//suppress MIC calculation
		} else {
			ifbp->IFB_MICRxCarry = 0;
//*	Note that "coincidentally" the bit positions used in HFS_STAT
//*	correspond with the offset of the key in IFB_MICKey
			i = ( i & HFS_STAT_MIC_KEY_ID ) >> 10;	/* coincidentally no shift needed for i itself */
			ifbp->IFB_MICRx[0] = CNV_LONG_TO_LITTLE(ifbp->IFB_MICRxKey[i  ]);
			ifbp->IFB_MICRx[1] = CNV_LONG_TO_LITTLE(ifbp->IFB_MICRxKey[i+1]);
		}
	} else {
		if ( ifbp->IFB_MICRxCarry == 0 ) {
			x.x32 = CNV_LONGP_TO_LITTLE(p);
			p += 4;
			if ( len < 4 ) {
				ifbp->IFB_MICRxCarry = (hcf_16)len;
			} else {
				ifbp->IFB_MICRxCarry = 4;
				len -= 4;
			}
		} else while ( ifbp->IFB_MICRxCarry < 4 && len ) {		//note for hcf_16 applies: 0xFFFF > 4
			x.x8[ifbp->IFB_MICRxCarry++] = *p++;
			len--;
		}
		while ( ifbp->IFB_MICRxCarry == 4 ) {	//contrived so we have only 1 call to calc_mic so we could bring it in-line
			calc_mic( ifbp->IFB_MICRx, x.x32 );
			x.x32 = CNV_LONGP_TO_LITTLE(p);
			p += 4;
			if ( len < 4 ) {
				ifbp->IFB_MICRxCarry = (hcf_16)len;
			}
			len -= 4;
		}
	}
} // calc_mic_rx_frag
#endif // HCF_TYPE_WPA


#if (HCF_TYPE) & HCF_TYPE_WPA
void
calc_mic_tx_frag( IFBP ifbp, wci_bufp p, int len )
{
static union { hcf_32 x32; hcf_16 x16[2]; hcf_8 x8[4]; } x;	//* area to accumulate 4 bytes input for MIC engine

														//if initialization request
	if ( len == -1 ) {
														//.  presume MIC calculation disabled
		ifbp->IFB_MICTxCarry = 0xFFFF;
														//.  if MIC calculation enabled
		if ( ifbp->IFB_MICTxCntl ) {
														//.  .  clear MIC carry
			ifbp->IFB_MICTxCarry = 0;
														//.  .  initialize MIC-engine
			ifbp->IFB_MICTx[0] = CNV_LONG_TO_LITTLE(ifbp->IFB_MICTxKey[0]);	/*Tx always uses Key 0 */
			ifbp->IFB_MICTx[1] = CNV_LONG_TO_LITTLE(ifbp->IFB_MICTxKey[1]);
		}
														//else
	} else {
														//.  if MIC enabled (Tx) / if MIC present (Rx)
														//.  and no carry from previous calc_mic_frag
		if ( ifbp->IFB_MICTxCarry == 0 ) {
														//.  .  preset accu with 4 bytes from buffer
			x.x32 = CNV_LONGP_TO_LITTLE(p);
														//.  .  adjust pointer accordingly
			p += 4;
														//.  .  if buffer contained less then 4 bytes
			if ( len < 4 ) {
														//.  .  .  promote valid bytes in accu to carry
														//.  .  .  flag accu to contain incomplete double word
				ifbp->IFB_MICTxCarry = (hcf_16)len;
														//.  .  else
			} else {
														//.  .  .  flag accu to contain complete double word
				ifbp->IFB_MICTxCarry = 4;
														//.  .  adjust remaining buffer length
				len -= 4;
			}
														//.  else if MIC enabled
														//.  and if carry bytes from previous calc_mic_tx_frag
														//.  .  move (1-3) bytes from carry into accu
		} else while ( ifbp->IFB_MICTxCarry < 4 && len ) {		/* note for hcf_16 applies: 0xFFFF > 4 */
			x.x8[ifbp->IFB_MICTxCarry++] = *p++;
			len--;
		}
														//.  while accu contains complete double word
														//.  and MIC enabled
		while ( ifbp->IFB_MICTxCarry == 4 ) {
														//.  .  pass accu to MIC engine
			calc_mic( ifbp->IFB_MICTx, x.x32 );
														//.  .  copy next 4 bytes from buffer to accu
			x.x32 = CNV_LONGP_TO_LITTLE(p);
														//.  .  adjust buffer pointer
			p += 4;
														//.  .  if buffer contained less then 4 bytes
														//.  .  .  promote valid bytes in accu to carry
														//.  .  .  flag accu to contain incomplete double word
			if ( len < 4 ) {
				ifbp->IFB_MICTxCarry = (hcf_16)len;
			}
														//.  .  adjust remaining buffer length
			len -= 4;
		}
	}
} // calc_mic_tx_frag
#endif // HCF_TYPE_WPA


#if HCF_PROT_TIME
HCF_STATIC void
calibrate( IFBP ifbp )
{
int		cnt = HCF_PROT_TIME_CNT;
hcf_32	prot_cnt;

	HCFTRACE( ifbp, HCF_TRACE_CALIBRATE );
	if ( ifbp->IFB_TickIni == INI_TICK_INI ) {													/*1*/
		ifbp->IFB_TickIni = 0;																	/*2*/
			while ( cnt-- ) {
				prot_cnt = INI_TICK_INI;
				OPW( HREG_EV_ACK, HREG_EV_TICK );												/*3*/
				while ( (IPW( HREG_EV_STAT ) & HREG_EV_TICK) == 0 && --prot_cnt ) {
					ifbp->IFB_TickIni++;
				}
				if ( prot_cnt == 0 || prot_cnt == INI_TICK_INI ) {								/*4*/
					ifbp->IFB_TickIni = INI_TICK_INI;
					ifbp->IFB_DefunctStat = HCF_ERR_DEFUNCT_TIMER;
					ifbp->IFB_CardStat |= CARD_STAT_DEFUNCT;
					HCFASSERT( DO_ASSERT, prot_cnt )
				}
			}
		ifbp->IFB_TickIni <<= HCF_PROT_TIME_SHFT;												/*8*/
	}
	HCFTRACE( ifbp, HCF_TRACE_CALIBRATE | HCF_TRACE_EXIT );
} // calibrate
#endif // HCF_PROT_TIME


#if (HCF_DL_ONLY) == 0
#if (HCF_TYPE) & HCF_TYPE_WPA
int
check_mic( IFBP ifbp )
{
int		rc = HCF_SUCCESS;
hcf_32 x32[2];				//* area to save rcvd 8 bytes MIC

													//if MIC present in RxFS
	if ( *(wci_recordp)&ifbp->IFB_lap[-HFS_ADDR_DEST] & HFS_STAT_MIC ) {
	//or if ( ifbp->IFB_MICRxCarry != 0xFFFF )
		CALC_RX_MIC( mic_pad, 8 );					//.  process up to 3 remaining bytes of data and append 5 to 8 bytes of padding to MIC calculation
		get_frag( ifbp, (wci_bufp)x32, 8 BE_PAR(0));//.  get 8 byte MIC from NIC
													//.  if calculated and received MIC do not match
													//.  .  set status at HCF_ERR_MIC
/*14*/  if ( x32[0] != CNV_LITTLE_TO_LONG(ifbp->IFB_MICRx[0]) ||
        	 x32[1] != CNV_LITTLE_TO_LONG(ifbp->IFB_MICRx[1])	  ) {
			rc = HCF_ERR_MIC;
		}
	}
													//return status
	return rc;
} // check_mic
#endif // HCF_TYPE_WPA
#endif // HCF_DL_ONLY


HCF_STATIC int
cmd_cmpl( IFBP ifbp )
{

PROT_CNT_INI
int		rc = HCF_SUCCESS;
hcf_16	stat;

	HCFLOGENTRY( HCF_TRACE_CMD_CPL, ifbp->IFB_Cmd )
	ifbp->IFB_Cmd &= ~HCMD_BUSY;												/* 2 */
	HCF_WAIT_WHILE( (IPW( HREG_EV_STAT) & HREG_EV_CMD) == 0 );					/* 4 */
	stat = IPW( HREG_STAT );
#if HCF_PROT_TIME
	if ( prot_cnt == 0 ) {
		IF_TALLY( ifbp->IFB_HCF_Tallies.MiscErr++; )
		rc = HCF_ERR_TIME_OUT;
		HCFASSERT( DO_ASSERT, ifbp->IFB_Cmd )
	} else
#endif // HCF_PROT_TIME
	{
		DAWA_ACK( HREG_EV_CMD );
/*4*/	if ( stat != (ifbp->IFB_Cmd & HCMD_CMD_CODE) ) {
/*8*/		if ( ( (stat ^ ifbp->IFB_Cmd ) & HCMD_CMD_CODE) != 0 ) {
				rc = ifbp->IFB_DefunctStat = HCF_ERR_DEFUNCT_CMD_SEQ;
				ifbp->IFB_CardStat |= CARD_STAT_DEFUNCT;
			}
			IF_TALLY( ifbp->IFB_HCF_Tallies.MiscErr++; )
			ifbp->IFB_ErrCmd = stat;
			ifbp->IFB_ErrQualifier = IPW( HREG_RESP_0 );
			HCFASSERT( DO_ASSERT, MERGE_2( IPW( HREG_PARAM_0 ), ifbp->IFB_Cmd ) )
			HCFASSERT( DO_ASSERT, MERGE_2( ifbp->IFB_ErrQualifier, ifbp->IFB_ErrCmd ) )
		}
	}
	HCFASSERT( rc == HCF_SUCCESS, rc)
	HCFLOGEXIT( HCF_TRACE_CMD_CPL )
	return rc;
} // cmd_cmpl



HCF_STATIC int
cmd_exe( IFBP ifbp, hcf_16 cmd_code, hcf_16 par_0 )	//if HCMD_BUSY of cmd_code set, then do NOT wait for completion
{
int rc;

	HCFLOGENTRY( HCF_TRACE_CMD_EXE, cmd_code )
	HCFASSERT( (cmd_code & HCMD_CMD_CODE) != HCMD_TX || cmd_code & HCMD_BUSY, cmd_code ) //Tx must have Busy bit set
	OPW( HREG_SW_0, HCF_MAGIC );
	if ( IPW( HREG_SW_0 ) == HCF_MAGIC ) {														/* 1 */
		rc = ifbp->IFB_DefunctStat;
	}
	else rc = HCF_ERR_NO_NIC;
	if ( rc == HCF_SUCCESS ) {
		//;?is this a hot idea, better MEASURE performance impact
/*2*/	if ( ifbp->IFB_Cmd & HCMD_BUSY ) {
			rc = cmd_cmpl( ifbp );
		}
		OPW( HREG_PARAM_0, par_0 );
		OPW( HREG_CMD, cmd_code &~HCMD_BUSY );
		ifbp->IFB_Cmd = cmd_code;
		if ( (cmd_code & HCMD_BUSY) == 0 ) {	//;?is this a hot idea, better MEASURE performance impact
			rc = cmd_cmpl( ifbp );
		}
	}
	HCFASSERT( rc == HCF_SUCCESS, MERGE_2( rc, cmd_code ) )
	HCFLOGEXIT( HCF_TRACE_CMD_EXE )
	return rc;
} // cmd_exe


HCF_STATIC int
download( IFBP ifbp, CFG_PROG_STRCT FAR *ltvp )						//Hermes-II download (volatile only)
{
hcf_16				i;
int					rc = HCF_SUCCESS;
wci_bufp			cp;
hcf_io				io_port = ifbp->IFB_IOBase + HREG_AUX_DATA;

	HCFLOGENTRY( HCF_TRACE_DL, ltvp->typ )
#if (HCF_TYPE) & HCF_TYPE_PRELOADED
	HCFASSERT( DO_ASSERT, ltvp->mode )
#else
													//if initial "program" LTV
	if ( ifbp->IFB_DLMode == CFG_PROG_STOP && ltvp->mode == CFG_PROG_VOLATILE) {
													//.  switch Hermes to initial mode
/*1*/	OPW( HREG_EV_ACK, ~HREG_EV_SLEEP_REQ );
		rc = cmd_exe( ifbp, HCMD_INI, 0 );	/* HCMD_INI can not be part of init() because that is called on
											 * other occasions as well */
		rc = init( ifbp );
	}
													//if final "program" LTV
	if ( ltvp->mode == CFG_PROG_STOP && ifbp->IFB_DLMode == CFG_PROG_VOLATILE) {
													//.  start tertiary (or secondary)
		OPW( HREG_PARAM_1, (hcf_16)(ltvp->nic_addr >> 16) );
		rc = cmd_exe( ifbp, HCMD_EXECUTE, (hcf_16) ltvp->nic_addr );
		if (rc == HCF_SUCCESS) {
			rc = init( ifbp );	/*;? do we really want to skip init if cmd_exe failed, i.e.
								 *	 IFB_FW_Comp_Id is than possibly incorrect */
	  	}
													//else (non-final)
	} else {
													//.  if mode == Readback SEEPROM
#if 0	//;? as long as the next if contains a hard coded 0, might as well leave it out even more obvious
		if ( 0 /*len is definitely not want we want;?*/ && ltvp->mode == CFG_PROG_SEEPROM_READBACK ) {
			OPW( HREG_PARAM_1, (hcf_16)(ltvp->nic_addr >> 16) );
			OPW( HREG_PARAM_2, MUL_BY_2(ltvp->len - 4));
													//.  .  perform Hermes prog cmd with appropriate mode bits
			rc = cmd_exe( ifbp, HCMD_PROGRAM | ltvp->mode, (hcf_16)ltvp->nic_addr );
													//.  .  set up NIC RAM addressability according Resp0-1
			OPW( HREG_AUX_PAGE,   IPW( HREG_RESP_1) );
			OPW( HREG_AUX_OFFSET, IPW( HREG_RESP_0) );
													//.  .  set up L-field of LTV according Resp2
			i = ( IPW( HREG_RESP_2 ) + 1 ) / 2;  // i contains max buffer size in words, a probably not very useful piece of information ;?
													//.  .  copy data from NIC via AUX port to LTV
			cp = (wci_bufp)ltvp->host_addr;						/*IN_PORT_STRING_8_16 macro may modify its parameters*/
			i = ltvp->len - 4;
			IN_PORT_STRING_8_16( io_port, cp, i );		//!!!WORD length, cp MUST be a char pointer	// $$ char
													//.  else (non-final programming)
		} else
#endif //;? as long as the above if contains a hard coded 0, might as well leave it out even more obvious
		{											//.  .  get number of words to program
			HCFASSERT( ltvp->segment_size, *ltvp->host_addr )
			i = ltvp->segment_size/2;
													//.  .  copy data (words) from LTV via AUX port to NIC
			cp = (wci_bufp)ltvp->host_addr;						//OUT_PORT_STRING_8_16 macro may modify its parameters
													//.  .  if mode == volatile programming
			if ( ltvp->mode == CFG_PROG_VOLATILE ) {
													//.  .  .  set up NIC RAM addressability via AUX port
				OPW( HREG_AUX_PAGE, (hcf_16)(ltvp->nic_addr >> 16 << 9 | (ltvp->nic_addr & 0xFFFF) >> 7 ) );
				OPW( HREG_AUX_OFFSET, (hcf_16)(ltvp->nic_addr & 0x007E) );
				OUT_PORT_STRING_8_16( io_port, cp, i );		//!!!WORD length, cp MUST be a char pointer
			}
		}
	}
	ifbp->IFB_DLMode = ltvp->mode;					//save state in IFB_DLMode
#endif // HCF_TYPE_PRELOADED
	HCFASSERT( rc == HCF_SUCCESS, rc )
	HCFLOGEXIT( HCF_TRACE_DL )
	return rc;
} // download


#if (HCF_ASSERT) & HCF_ASSERT_PRINTF
HCF_STATIC int
fw_printf(IFBP ifbp, CFG_FW_PRINTF_STRCT FAR *ltvp)
{
    int rc = HCF_SUCCESS;
    hcf_16 fw_cnt;
//    hcf_32 DbMsgBuffer = 0x29D2, DbMsgCount= 0x000029D0;
//    hcf_16 DbMsgSize=0x00000080;
    hcf_32 DbMsgBuffer;
    CFG_FW_PRINTF_BUFFER_LOCATION_STRCT *p = &ifbp->IFB_FwPfBuff;
    ltvp->len = 1;
    if ( p->DbMsgSize != 0 ) {
        // first, check the counter in nic-RAM and compare it to the latest counter value of the HCF
        OPW( HREG_AUX_PAGE, (hcf_16)(p->DbMsgCount >> 7) );
        OPW( HREG_AUX_OFFSET, (hcf_16)(p->DbMsgCount & 0x7E) );
        fw_cnt = ((IPW( HREG_AUX_DATA) >>1 ) & ((hcf_16)p->DbMsgSize - 1));
        if ( fw_cnt != ifbp->IFB_DbgPrintF_Cnt ) {
//    DbgPrint("fw_cnt=%d IFB_DbgPrintF_Cnt=%d\n", fw_cnt, ifbp->IFB_DbgPrintF_Cnt);
            DbMsgBuffer = p->DbMsgBuffer + ifbp->IFB_DbgPrintF_Cnt * 6; // each entry is 3 words
            OPW( HREG_AUX_PAGE, (hcf_16)(DbMsgBuffer >> 7) );
            OPW( HREG_AUX_OFFSET, (hcf_16)(DbMsgBuffer & 0x7E) );
            ltvp->msg_id     = IPW(HREG_AUX_DATA);
            ltvp->msg_par    = IPW(HREG_AUX_DATA);
            ltvp->msg_tstamp = IPW(HREG_AUX_DATA);
            ltvp->len = 4;
            ifbp->IFB_DbgPrintF_Cnt++;
            ifbp->IFB_DbgPrintF_Cnt &= (p->DbMsgSize - 1);
        }
    }
    return rc;
};
#endif // HCF_ASSERT_PRINTF


#if (HCF_DL_ONLY) == 0
HCF_STATIC hcf_16
get_fid( IFBP ifbp )
{

hcf_16 fid = 0;
#if ( (HCF_TYPE) & HCF_TYPE_HII5 ) == 0
PROT_CNT_INI
#endif // HCF_TYPE_HII5

	IF_DMA( HCFASSERT(!(ifbp->IFB_CntlOpt & USE_DMA), ifbp->IFB_CntlOpt) )

	if ( IPW( HREG_EV_STAT) & HREG_EV_ALLOC) {
		fid = IPW( HREG_ALLOC_FID );
		HCFASSERT( fid, ifbp->IFB_RscInd )
		DAWA_ZERO_FID( HREG_ALLOC_FID )
#if ( (HCF_TYPE) & HCF_TYPE_HII5 ) == 0
		HCF_WAIT_WHILE( ( IPW( HREG_EV_STAT ) & HREG_EV_ACK_REG_READY ) == 0 );
		HCFASSERT( prot_cnt, IPW( HREG_EV_STAT ) )
#endif // HCF_TYPE_HII5
		DAWA_ACK( HREG_EV_ALLOC );			//!!note that HREG_EV_ALLOC is written only once
// 180 degree error in logic ;? #if ALLOC_15
		if ( ifbp->IFB_RscInd == 1 ) {
			ifbp->IFB_RscInd = 0;
		}
//#endif // ALLOC_15
	} else {
// 180 degree error in logic ;? #if ALLOC_15
		fid = ifbp->IFB_RscInd;
//#endif // ALLOC_15
		ifbp->IFB_RscInd = 0;
	}
	return fid;
} // get_fid
#endif // HCF_DL_ONLY


HCF_STATIC void
get_frag( IFBP ifbp, wci_bufp bufp, int len BE_PAR( int word_len ) )
{
hcf_io		io_port = ifbp->IFB_IOBase + HREG_DATA_1;	//BAP data register
wci_bufp	p = bufp;									//working pointer
int			i;											//prevent side effects from macro
int			j;

	HCFASSERT( ((hcf_32)bufp & (HCF_ALIGN-1) ) == 0, (hcf_32)bufp )


	i = len;
													//if buffer length > 0 and carry from previous get_frag
	if ( i && ifbp->IFB_CarryIn ) {
													//.  move carry to buffer
													//.  adjust buffer length and pointer accordingly
		*p++ = (hcf_8)(ifbp->IFB_CarryIn>>8);
		i--;
													//.  clear carry flag
		ifbp->IFB_CarryIn = 0;
	}
#if (HCF_IO) & HCF_IO_32BITS
	//skip zero-length I/O, single byte I/O and I/O not worthwhile (i.e. less than 6 bytes)for DW logic
													//if buffer length >= 6 and 32 bits I/O support
	if ( !(ifbp->IFB_CntlOpt & USE_16BIT) && i >= 6 ) {
hcf_32 FAR	*p4; //prevent side effects from macro
		if ( ( (hcf_32)p & 0x1 ) == 0 ) {			//.  if buffer at least word aligned
			if ( (hcf_32)p & 0x2 ) {				//.  .  if buffer not double word aligned
													//.  .  .  read single word to get double word aligned
				*(wci_recordp)p = IN_PORT_WORD( io_port );
													//.  .  .  adjust buffer length and pointer accordingly
				p += 2;
				i -= 2;
			}
													//.  .  read as many double word as possible
			p4 = (hcf_32 FAR *)p;
			j = i/4;
			IN_PORT_STRING_32( io_port, p4, j );
													//.  .  adjust buffer length and pointer accordingly
			p += i & ~0x0003;
			i &= 0x0003;
		}
	}
#endif // HCF_IO_32BITS
													//if no 32-bit support OR byte aligned OR 1-3 bytes left
	if ( i ) {
													//.  read as many word as possible in "alignment safe" way
		j = i/2;
		IN_PORT_STRING_8_16( io_port, p, j );
													//.  if 1 byte left
		if ( i & 0x0001 ) {
													//.  .  read 1 word
			ifbp->IFB_CarryIn = IN_PORT_WORD( io_port );
													//.  .  store LSB in last char of buffer
			bufp[len-1] = (hcf_8)ifbp->IFB_CarryIn;
													//.  .  save MSB in carry, set carry flag
			ifbp->IFB_CarryIn |= 0x1;
		}
	}
#if HCF_BIG_ENDIAN
	HCFASSERT( word_len == 0 || word_len == 2 || word_len == 4, word_len )
	HCFASSERT( word_len == 0 || ((hcf_32)bufp & 1 ) == 0, (hcf_32)bufp )
	HCFASSERT( word_len <= len, MERGE2( word_len, len ) )
	//see put_frag for an alternative implementation, but be carefull about what are int's and what are
	//hcf_16's
	if ( word_len ) {								//.  if there is anything to convert
hcf_8 c;
		c = bufp[1];								//.  .  convert the 1st hcf_16
		bufp[1] = bufp[0];
		bufp[0] = c;
		if ( word_len > 1 ) {						//.  .  if there is to convert more than 1 word ( i.e 2 )
			c = bufp[3];							//.  .  .  convert the 2nd hcf_16
			bufp[3] = bufp[2];
			bufp[2] = c;
		}
	}
#endif // HCF_BIG_ENDIAN
} // get_frag

HCF_STATIC int
init( IFBP ifbp )
{

int	rc = HCF_SUCCESS;

	HCFLOGENTRY( HCF_TRACE_INIT, 0 )

	ifbp->IFB_CardStat = 0;																			/* 2*/
	OPW( HREG_EV_ACK, ~HREG_EV_SLEEP_REQ ); 											/* 4*/
	IF_PROT_TIME( calibrate( ifbp ); ) 													/*10*/
#if 0 // OOR
	ifbp->IFB_FWIdentity.len = 2;							//misuse the IFB space for a put
	ifbp->IFB_FWIdentity.typ = CFG_TICK_TIME;
	ifbp->IFB_FWIdentity.comp_id = (1000*1000)/1024 + 1;	//roughly 1 second
	hcf_put_info( ifbp, (LTVP)&ifbp->IFB_FWIdentity.len );
#endif // OOR
	ifbp->IFB_FWIdentity.len = sizeof(CFG_FW_IDENTITY_STRCT)/sizeof(hcf_16) - 1;
	ifbp->IFB_FWIdentity.typ = CFG_FW_IDENTITY;
	rc = hcf_get_info( ifbp, (LTVP)&ifbp->IFB_FWIdentity.len );
/* ;? conversion should not be needed for mmd_check_comp */
#if HCF_BIG_ENDIAN
	ifbp->IFB_FWIdentity.comp_id       = CNV_LITTLE_TO_SHORT( ifbp->IFB_FWIdentity.comp_id );
	ifbp->IFB_FWIdentity.variant       = CNV_LITTLE_TO_SHORT( ifbp->IFB_FWIdentity.variant );
	ifbp->IFB_FWIdentity.version_major = CNV_LITTLE_TO_SHORT( ifbp->IFB_FWIdentity.version_major );
	ifbp->IFB_FWIdentity.version_minor = CNV_LITTLE_TO_SHORT( ifbp->IFB_FWIdentity.version_minor );
#endif // HCF_BIG_ENDIAN
#if defined MSF_COMPONENT_ID																		/*14*/
	if ( rc == HCF_SUCCESS ) {																		/*16*/
		ifbp->IFB_HSISup.len = sizeof(CFG_SUP_RANGE_STRCT)/sizeof(hcf_16) - 1;
		ifbp->IFB_HSISup.typ = CFG_NIC_HSI_SUP_RANGE;
		rc = hcf_get_info( ifbp, (LTVP)&ifbp->IFB_HSISup.len );
#if HCF_BIG_ENDIAN
		ifbp->IFB_HSISup.role    = CNV_LITTLE_TO_SHORT( ifbp->IFB_HSISup.role );
		ifbp->IFB_HSISup.id      = CNV_LITTLE_TO_SHORT( ifbp->IFB_HSISup.id );
		ifbp->IFB_HSISup.variant = CNV_LITTLE_TO_SHORT( ifbp->IFB_HSISup.variant );
		ifbp->IFB_HSISup.bottom  = CNV_LITTLE_TO_SHORT( ifbp->IFB_HSISup.bottom );
		ifbp->IFB_HSISup.top     = CNV_LITTLE_TO_SHORT( ifbp->IFB_HSISup.top );
#endif // HCF_BIG_ENDIAN
		ifbp->IFB_FWSup.len = sizeof(CFG_SUP_RANGE_STRCT)/sizeof(hcf_16) - 1;
		ifbp->IFB_FWSup.typ = CFG_FW_SUP_RANGE;
		(void)hcf_get_info( ifbp, (LTVP)&ifbp->IFB_FWSup.len );
/* ;? conversion should not be needed for mmd_check_comp */
#if HCF_BIG_ENDIAN
		ifbp->IFB_FWSup.role    = CNV_LITTLE_TO_SHORT( ifbp->IFB_FWSup.role );
		ifbp->IFB_FWSup.id      = CNV_LITTLE_TO_SHORT( ifbp->IFB_FWSup.id );
		ifbp->IFB_FWSup.variant = CNV_LITTLE_TO_SHORT( ifbp->IFB_FWSup.variant );
		ifbp->IFB_FWSup.bottom  = CNV_LITTLE_TO_SHORT( ifbp->IFB_FWSup.bottom );
		ifbp->IFB_FWSup.top     = CNV_LITTLE_TO_SHORT( ifbp->IFB_FWSup.top );
#endif // HCF_BIG_ENDIAN

		if ( ifbp->IFB_FWSup.id == COMP_ID_PRI ) {												/* 20*/
int i = sizeof( CFG_FW_IDENTITY_STRCT) + sizeof(CFG_SUP_RANGE_STRCT );
			while ( i-- ) ((hcf_8*)(&ifbp->IFB_PRIIdentity))[i] = ((hcf_8*)(&ifbp->IFB_FWIdentity))[i];
			ifbp->IFB_PRIIdentity.typ = CFG_PRI_IDENTITY;
			ifbp->IFB_PRISup.typ = CFG_PRI_SUP_RANGE;
			xxxx[xxxx_PRI_IDENTITY_OFFSET] = &ifbp->IFB_PRIIdentity.len;
			xxxx[xxxx_PRI_IDENTITY_OFFSET+1] = &ifbp->IFB_PRISup.len;
		}
		if ( !mmd_check_comp( (void*)&cfg_drv_act_ranges_hsi, &ifbp->IFB_HSISup)				 /* 22*/
#if ( (HCF_TYPE) & HCF_TYPE_PRELOADED ) == 0
//;? the PRI compatibility check is only relevant for DHF
			 || !mmd_check_comp( (void*)&cfg_drv_act_ranges_pri, &ifbp->IFB_PRISup)
#endif // HCF_TYPE_PRELOADED
		   ) {
			ifbp->IFB_CardStat = CARD_STAT_INCOMP_PRI;
			rc = HCF_ERR_INCOMP_PRI;
		}
		if ( ( ifbp->IFB_FWSup.id == COMP_ID_STA &&	!mmd_check_comp( (void*)&cfg_drv_act_ranges_sta, &ifbp->IFB_FWSup) ) ||
			 ( ifbp->IFB_FWSup.id == COMP_ID_APF && !mmd_check_comp( (void*)&cfg_drv_act_ranges_apf, &ifbp->IFB_FWSup) )
		   ) {																					/* 24 */
			ifbp->IFB_CardStat |= CARD_STAT_INCOMP_FW;
			rc = HCF_ERR_INCOMP_FW;
		}
	}
#endif // MSF_COMPONENT_ID
#if (HCF_DL_ONLY) == 0																			/* 28 */
	if ( rc == HCF_SUCCESS && ifbp->IFB_FWIdentity.comp_id >= COMP_ID_FW_STA ) {
PROT_CNT_INI
		/**************************************************************************************
		* rlav: the DMA engine needs the host to cause a 'hanging alloc event' for it to consume.
		* not sure if this is the right spot in the HCF, thinking about hcf_enable...
		**************************************************************************************/
		rc = cmd_exe( ifbp, HCMD_ALLOC, 0 );
// 180 degree error in logic ;? #if ALLOC_15
//		ifbp->IFB_RscInd = 1;	//let's hope that by the time hcf_send_msg isa called, there will be a FID
//#else
		if ( rc == HCF_SUCCESS ) {
			HCF_WAIT_WHILE( (IPW( HREG_EV_STAT ) & HREG_EV_ALLOC) == 0 );
			IF_PROT_TIME( HCFASSERT(prot_cnt, IPW( HREG_EV_STAT ) ) /*NOP*/;)
#if HCF_DMA
			if ( ! ( ifbp->IFB_CntlOpt & USE_DMA ) )
#endif // HCF_DMA
			{
				ifbp->IFB_RscInd = get_fid( ifbp );
				HCFASSERT( ifbp->IFB_RscInd, 0 )
				cmd_exe( ifbp, HCMD_ALLOC, 0 );
				IF_PROT_TIME( if ( prot_cnt == 0 ) rc = HCF_ERR_TIME_OUT; )
			}
		}
//#endif // ALLOC_15
	}
#endif // HCF_DL_ONLY
	HCFASSERT( rc == HCF_SUCCESS, rc )
	HCFLOGEXIT( HCF_TRACE_INIT )
	return rc;
} // init

#if (HCF_DL_ONLY) == 0
HCF_STATIC void
isr_info( IFBP ifbp )
{
hcf_16	info[2], fid;
#if (HCF_EXT) & HCF_EXT_INFO_LOG
RID_LOGP	ridp = ifbp->IFB_RIDLogp;	//NULL or pointer to array of RID_LOG structures (terminated by zero typ)
#endif // HCF_EXT_INFO_LOG

	HCFTRACE( ifbp, HCF_TRACE_ISR_INFO );																/* 1 */
	fid = IPW( HREG_INFO_FID );
	DAWA_ZERO_FID( HREG_INFO_FID )
	if ( fid ) {
		(void)setup_bap( ifbp, fid, 0, IO_IN );
		get_frag( ifbp, (wci_bufp)info, 4 BE_PAR(2) );
		HCFASSERT( info[0] <= HCF_MAX_LTV + 1, MERGE_2( info[1], info[0] ) )  //;? a smaller value makes more sense
#if (HCF_TALLIES) & HCF_TALLIES_NIC		//Hermes tally support
		if ( info[1] == CFG_TALLIES ) {
hcf_32	*p;
/*2*/		if ( info[0] > HCF_NIC_TAL_CNT ) {
				info[0] = HCF_NIC_TAL_CNT + 1;
			}
			p = (hcf_32*)&ifbp->IFB_NIC_Tallies;
			while ( info[0]-- >1 ) *p++ += IPW( HREG_DATA_1 );	//request may return zero length
		}
		else
#endif // HCF_TALLIES_NIC
		{
/*4*/		if ( info[1] == CFG_LINK_STAT ) {
				ifbp->IFB_LinkStat = IPW( HREG_DATA_1 );
			}
#if (HCF_EXT) & HCF_EXT_INFO_LOG
/*6*/		while ( 1 ) {
				if ( ridp->typ == 0 || ridp->typ == info[1] ) {
					if ( ridp->bufp ) {
						HCFASSERT( ridp->len >= 2, ridp->typ )
						ridp->bufp[0] = min((hcf_16)(ridp->len - 1), info[0] ); 	//save L
						ridp->bufp[1] = info[1];						//save T
						get_frag( ifbp, (wci_bufp)&ridp->bufp[2], (ridp->bufp[0] - 1)*2 BE_PAR(0) );
					}
					break;
				}
				ridp++;
			}
#endif // HCF_EXT_INFO_LOG
		}
		HCFTRACE( ifbp, HCF_TRACE_ISR_INFO | HCF_TRACE_EXIT );
	}
	return;
} // isr_info
#endif // HCF_DL_ONLY

//
//
// #endif // HCF_TALLIES_NIC
// /*4*/	if ( info[1] == CFG_LINK_STAT ) {
// 			ifbp->IFB_DSLinkStat = IPW( HREG_DATA_1 ) | CFG_LINK_STAT_CHANGE;	//corrupts BAP !! ;?
// 			ifbp->IFB_LinkStat = ifbp->IFB_DSLinkStat & CFG_LINK_STAT_FW; //;? to be obsoleted
// 			printk( "<4>linkstatus: %04x\n", ifbp->IFB_DSLinkStat );		//;?remove me 1 day
// #if (HCF_SLEEP) & HCF_DDS
// 			if ( ( ifbp->IFB_DSLinkStat & CFG_LINK_STAT_CONNECTED ) == 0 ) { 	//even values are disconnected etc.
// 				ifbp->IFB_TickCnt = 0;				//start 2 second period (with 1 tick uncertanty)
// 				printk( "<5>isr_info: AwaitConnection phase started, IFB_TickCnt = 0\n" );		//;?remove me 1 day
// 			}
// #endif // HCF_DDS
// 		}
// #if (HCF_EXT) & HCF_EXT_INFO_LOG
// /*6*/	while ( 1 ) {
// 			if ( ridp->typ == 0 || ridp->typ == info[1] ) {
// 				if ( ridp->bufp ) {
// 					HCFASSERT( ridp->len >= 2, ridp->typ )
// 					(void)setup_bap( ifbp, fid, 2, IO_IN );			//restore BAP for tallies, linkstat and specific type followed by wild card
// 					ridp->bufp[0] = min( ridp->len - 1, info[0] ); 	//save L
// 					get_frag( ifbp, (wci_bufp)&ridp->bufp[1], ridp->bufp[0]*2 BE_PAR(0) );
// 				}
// 				break; //;?this break is no longer needed due to setup_bap but lets concentrate on DDS first
// 			}
// 			ridp++;
// 		}
// #endif // HCF_EXT_INFO_LOG
// 	}
// 	HCFTRACE( ifbp, HCF_TRACE_ISR_INFO | HCF_TRACE_EXIT );
//
//
//
//
//	return;
//} // isr_info
//#endif // HCF_DL_ONLY


#if HCF_ASSERT
void
mdd_assert( IFBP ifbp, unsigned int line_number, hcf_32 q )
{
hcf_16	run_time_flag = ifbp->IFB_AssertLvl;

	if ( run_time_flag /* > ;?????? */ ) { //prevent recursive behavior, later to be extended to level filtering
		ifbp->IFB_AssertQualifier = q;
		ifbp->IFB_AssertLine = (hcf_16)line_number;
#if (HCF_ASSERT) & ( HCF_ASSERT_LNK_MSF_RTN | HCF_ASSERT_RT_MSF_RTN )
		if ( ifbp->IFB_AssertRtn ) {
			ifbp->IFB_AssertRtn( line_number, ifbp->IFB_AssertTrace, q );
		}
#endif // HCF_ASSERT_LNK_MSF_RTN / HCF_ASSERT_RT_MSF_RTN
#if (HCF_ASSERT) & HCF_ASSERT_SW_SUP
		OPW( HREG_SW_2, line_number );
		OPW( HREG_SW_2, ifbp->IFB_AssertTrace );
		OPW( HREG_SW_2, (hcf_16)q );
		OPW( HREG_SW_2, (hcf_16)(q >> 16 ) );
#endif // HCF_ASSERT_SW_SUP

#if (HCF_EXT) & HCF_EXT_MB && (HCF_ASSERT) & HCF_ASSERT_MB
		ifbp->IFB_AssertLvl = 0;									// prevent recursive behavior
		hcf_put_info( ifbp, (LTVP)&ifbp->IFB_AssertStrct );
		ifbp->IFB_AssertLvl = run_time_flag;						// restore appropriate filter level
#endif // HCF_EXT_MB / HCF_ASSERT_MB
	}
} // mdd_assert
#endif // HCF_ASSERT


HCF_STATIC void
put_frag( IFBP ifbp, wci_bufp bufp, int len BE_PAR( int word_len ) )
{
hcf_io		io_port = ifbp->IFB_IOBase + HREG_DATA_1;	//BAP data register
int			i;											//prevent side effects from macro
hcf_16		j;
	HCFASSERT( ((hcf_32)bufp & (HCF_ALIGN-1) ) == 0, (hcf_32)bufp )
#if HCF_BIG_ENDIAN
	HCFASSERT( word_len == 0 || word_len == 2 || word_len == 4, word_len )
	HCFASSERT( word_len == 0 || ((hcf_32)bufp & 1 ) == 0, (hcf_32)bufp )
	HCFASSERT( word_len <= len, MERGE_2( word_len, len ) )

	if ( word_len ) {									//if there is anything to convert
 														//.  convert and write the 1st hcf_16
		j = bufp[1] | bufp[0]<<8;
		OUT_PORT_WORD( io_port, j );
														//.  update pointer and counter accordingly
		len -= 2;
		bufp += 2;
		if ( word_len > 1 ) {							//.  if there is to convert more than 1 word ( i.e 2 )
 														//.  .  convert and write the 2nd hcf_16
			j = bufp[1] | bufp[0]<<8;	/*bufp is already incremented by 2*/
			OUT_PORT_WORD( io_port, j );
														//.  .  update pointer and counter accordingly
			len -= 2;
			bufp += 2;
		}
	}
#endif // HCF_BIG_ENDIAN
	i = len;
	if ( i && ifbp->IFB_CarryOut ) {					//skip zero-length
		j = ((*bufp)<<8) + ( ifbp->IFB_CarryOut & 0xFF );
		OUT_PORT_WORD( io_port, j );
		bufp++; i--;
		ifbp->IFB_CarryOut = 0;
	}
#if (HCF_IO) & HCF_IO_32BITS
	//skip zero-length I/O, single byte I/O and I/O not worthwhile (i.e. less than 6 bytes)for DW logic
													//if buffer length >= 6 and 32 bits I/O support
	if ( !(ifbp->IFB_CntlOpt & USE_16BIT) && i >= 6 ) {
hcf_32 FAR	*p4; //prevent side effects from macro
		if ( ( (hcf_32)bufp & 0x1 ) == 0 ) {			//.  if buffer at least word aligned
			if ( (hcf_32)bufp & 0x2 ) {				//.  .  if buffer not double word aligned
                                                 	//.  .  .  write a single word to get double word aligned
				j = *(wci_recordp)bufp;		//just to help ease writing macros with embedded assembly
				OUT_PORT_WORD( io_port, j );
													//.  .  .  adjust buffer length and pointer accordingly
				bufp += 2; i -= 2;
			}
													//.  .  write as many double word as possible
			p4 = (hcf_32 FAR *)bufp;
			j = (hcf_16)i/4;
			OUT_PORT_STRING_32( io_port, p4, j );
													//.  .  adjust buffer length and pointer accordingly
			bufp += i & ~0x0003;
			i &= 0x0003;
		}
	}
#endif // HCF_IO_32BITS
													//if no 32-bit support OR byte aligned OR 1 word left
	if ( i ) {
													//.  if odd number of bytes left
		if ( i & 0x0001 ) {
													//.  .  save left over byte (before bufp is corrupted) in carry, set carry flag
			ifbp->IFB_CarryOut = (hcf_16)bufp[i-1] | 0x0100;	//note that i and bufp are always simultaneously modified, &bufp[i-1] is invariant
		}
													//.  write as many word as possible in "alignment safe" way
		j = (hcf_16)i/2;
		OUT_PORT_STRING_8_16( io_port, bufp, j );
	}
} // put_frag


HCF_STATIC void
put_frag_finalize( IFBP ifbp )
{
#if (HCF_TYPE) & HCF_TYPE_WPA
	if ( ifbp->IFB_MICTxCarry != 0xFFFF) {		//if MIC calculation active
		CALC_TX_MIC( mic_pad, 8);				//.  feed (up to 8 bytes of) virtual padding to MIC engine
												//.  write (possibly) trailing byte + (most of) MIC
		put_frag( ifbp, (wci_bufp)ifbp->IFB_MICTx, 8 BE_PAR(0) );
	}
#endif // HCF_TYPE_WPA
	put_frag( ifbp, null_addr, 1 BE_PAR(0) );	//write (possibly) trailing data or MIC byte
} // put_frag_finalize


HCF_STATIC int
put_info( IFBP ifbp, LTVP ltvp	)
{

int rc = HCF_SUCCESS;

	HCFASSERT( ifbp->IFB_CardStat == 0, MERGE_2( ltvp->typ, ifbp->IFB_CardStat ) )
	HCFASSERT( CFG_RID_CFG_MIN <= ltvp->typ && ltvp->typ <= CFG_RID_CFG_MAX, ltvp->typ )

	if ( ifbp->IFB_CardStat == 0 &&																/* 20*/
		 ( ( CFG_RID_CFG_MIN <= ltvp->typ    && ltvp->typ <= CFG_RID_CFG_MAX ) ||
		   ( CFG_RID_ENG_MIN <= ltvp->typ /* && ltvp->typ <= 0xFFFF */       )     ) ) {
#if HCF_ASSERT //FCC8, FCB0, FCB4, FCB6, FCB7, FCB8, FCC0, FCC4, FCBC, FCBD, FCBE, FCBF
 {
 hcf_16		t = ltvp->typ;
 LTV_STRCT 	x = { 2, t, {0} };															/*24*/
	hcf_get_info( ifbp, (LTVP)&x );
	if ( x.len == 0 &&
		 ( t != CFG_DEFAULT_KEYS && t != CFG_ADD_TKIP_DEFAULT_KEY && t != CFG_REMOVE_TKIP_DEFAULT_KEY &&
		   t != CFG_ADD_TKIP_MAPPED_KEY && t != CFG_REMOVE_TKIP_MAPPED_KEY &&
		   t != CFG_HANDOVER_ADDR && t != CFG_DISASSOCIATE_ADDR &&
		   t != CFG_FCBC && t != CFG_FCBD && t != CFG_FCBE && t != CFG_FCBF &&
		   t != CFG_DEAUTHENTICATE_ADDR
		 )
		) {
		HCFASSERT( DO_ASSERT, ltvp->typ )
		}
 }
#endif // HCF_ASSERT

		rc = setup_bap( ifbp, ltvp->typ, 0, IO_OUT );
		put_frag( ifbp, (wci_bufp)ltvp, 2*ltvp->len + 2 BE_PAR(2) );
/*28*/	if ( rc == HCF_SUCCESS ) {
			rc = cmd_exe( ifbp, HCMD_ACCESS + HCMD_ACCESS_WRITE, ltvp->typ );
		}
	}
	return rc;
} // put_info


#if (HCF_DL_ONLY) == 0
#if (HCF_EXT) & HCF_EXT_MB

HCF_STATIC int
put_info_mb( IFBP ifbp, CFG_MB_INFO_STRCT FAR * ltvp )
{

int			rc = HCF_SUCCESS;
hcf_16		i;						//work counter
hcf_16		*dp;					//destination pointer (in MailBox)
wci_recordp	sp;						//source pointer
hcf_16		len;					//total length to copy to MailBox
hcf_16		tlen;					//free length/working length/offset in WMP frame

	if ( ifbp->IFB_MBp == NULL ) return rc;  //;?not sufficient
	HCFASSERT( ifbp->IFB_MBp != NULL, 0 )					//!!!be careful, don't get into an endless recursion
	HCFASSERT( ifbp->IFB_MBSize, 0 )

	len = 1;																							/* 1 */
	for ( i = 0; i < ltvp->frag_cnt; i++ ) {
		len += ltvp->frag_buf[i].frag_len;
	}
	if ( ifbp->IFB_MBRp > ifbp->IFB_MBWp ) {
		tlen = ifbp->IFB_MBRp - ifbp->IFB_MBWp;															/* 2a*/
	} else {
		if ( ifbp->IFB_MBRp == ifbp->IFB_MBWp ) {
			ifbp->IFB_MBRp = ifbp->IFB_MBWp = 0;	// optimize Wrapping
		}
		tlen = ifbp->IFB_MBSize - ifbp->IFB_MBWp;														/* 2b*/
		if ( ( tlen <= len + 2 ) && ( len + 2 < ifbp->IFB_MBRp ) ) {	//if trailing space is too small but
																	//	 leading space is sufficiently large
			ifbp->IFB_MBp[ifbp->IFB_MBWp] = 0xFFFF;					//flag dummy LTV to fill the trailing space
			ifbp->IFB_MBWp = 0;										//reset WritePointer to begin of MailBox
			tlen = ifbp->IFB_MBRp;									//get new available space size
		}
	}
	dp = &ifbp->IFB_MBp[ifbp->IFB_MBWp];
	if ( len == 0 ) {
		tlen = 0; //;? what is this good for
	}
	if ( len + 2 >= tlen ){																				/* 6 */
		//Do Not ASSERT, this is a normal condition
		IF_TALLY( ifbp->IFB_HCF_Tallies.NoBufMB++; ) /*NOP to cover against analomies with empty compound*/;
		rc = HCF_ERR_LEN;
	} else {
		*dp++ = len;									//write Len (= size of T+V in words to MB_Info block
		*dp++ = ltvp->base_typ;							//write Type to MB_Info block
		ifbp->IFB_MBWp += len + 1;						//update WritePointer of MailBox
		for ( i = 0; i < ltvp->frag_cnt; i++ ) {				// process each of the fragments
			sp = ltvp->frag_buf[i].frag_addr;
			len = ltvp->frag_buf[i].frag_len;
			while ( len-- ) *dp++ = *sp++;
		}
		ifbp->IFB_MBp[ifbp->IFB_MBWp] = 0;				//to assure get_info for CFG_MB_INFO stops
		ifbp->IFB_MBInfoLen = ifbp->IFB_MBp[ifbp->IFB_MBRp];											/* 8 */
	}
	return rc;
} // put_info_mb

#endif // HCF_EXT_MB
#endif // HCF_DL_ONLY


HCF_STATIC int
setup_bap( IFBP ifbp, hcf_16 fid, int offset, int type )
{
PROT_CNT_INI
int	rc;

	HCFTRACE( ifbp, HCF_TRACE_STRIO );
	rc = ifbp->IFB_DefunctStat;
	if (rc == HCF_SUCCESS) {										/*2*/
		OPW( HREG_SELECT_1, fid );																/*4*/
		OPW( HREG_OFFSET_1, offset );
		if ( type == IO_IN ) {
			ifbp->IFB_CarryIn = 0;
		}
		else ifbp->IFB_CarryOut = 0;
		HCF_WAIT_WHILE( IPW( HREG_OFFSET_1) & HCMD_BUSY );
		HCFASSERT( !( IPW( HREG_OFFSET_1) & HREG_OFFSET_ERR ), MERGE_2( fid, offset ) )			/*8*/
		if ( prot_cnt == 0 ) {
			HCFASSERT( DO_ASSERT, MERGE_2( fid, offset ) )
			rc = ifbp->IFB_DefunctStat = HCF_ERR_DEFUNCT_TIME_OUT;
			ifbp->IFB_CardStat |= CARD_STAT_DEFUNCT;
		}
	}
	HCFTRACE( ifbp, HCF_TRACE_STRIO | HCF_TRACE_EXIT );
	return rc;
} // setup_bap

