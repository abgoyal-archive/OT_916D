
#ifndef	_ASM_SN_KLCONFIG_H
#define	_ASM_SN_KLCONFIG_H



#include <linux/types.h>
#include <asm/sn/types.h>

#if defined(CONFIG_SGI_IP27)

#include <asm/sn/sn0/addrs.h>
//#include <sys/SN/router.h>
// XXX Stolen from <sys/SN/router.h>:
#define MAX_ROUTER_PORTS (6)    /* Max. number of ports on a router */
#include <asm/sn/fru.h>
//#include <sys/graph.h>
//#include <sys/xtalk/xbow.h>

#elif defined(CONFIG_SGI_IP35)

#include <asm/sn/sn1/addrs.h>
#include <sys/sn/router.h>
#include <sys/graph.h>
#include <asm/xtalk/xbow.h>

#endif /* !CONFIG_SGI_IP27 && !CONFIG_SGI_IP35 */

#if defined(CONFIG_SGI_IP27) || defined(CONFIG_SGI_IP35)
#include <asm/sn/agent.h>
#include <asm/fw/arc/types.h>
#include <asm/fw/arc/hinv.h>
#if defined(CONFIG_SGI_IP35)
// The hack file has to be before vector and after sn0_fru....
#include <asm/hack.h>
#include <asm/sn/vector.h>
#include <asm/xtalk/xtalk.h>
#endif /* CONFIG_SGI_IP35 */
#endif /* CONFIG_SGI_IP27 || CONFIG_SGI_IP35 */

typedef u64  nic_t;

#define KLCFGINFO_MAGIC	0xbeedbabe

typedef s32 klconf_off_t;

#define	MAX_MODULE_ID		255
#define SIZE_PAD		4096 /* 4k padding for structures */
#define MAX_SLOTS_PER_NODE	(1 + 2 + 6 + 2)

/* XXX if each node is guranteed to have some memory */

#define MAX_PCI_DEVS		8

/* lboard_t->brd_flags fields */

#define ENABLE_BOARD 		0x01
#define FAILED_BOARD  		0x02
#define DUPLICATE_BOARD 	0x04    /* Boards like midplanes/routers which
					   are discovered twice. Use one of them */
#define VISITED_BOARD		0x08	/* Used for compact hub numbering. */
#define LOCAL_MASTER_IO6	0x10 	/* master io6 for that node */
#define GLOBAL_MASTER_IO6	0x20
#define THIRD_NIC_PRESENT 	0x40  	/* for future use */
#define SECOND_NIC_PRESENT 	0x80 	/* addons like MIO are present */

/* klinfo->flags fields */

#define KLINFO_ENABLE 		0x01    /* This component is enabled */
#define KLINFO_FAILED   	0x02 	/* This component failed */
#define KLINFO_DEVICE   	0x04 	/* This component is a device */
#define KLINFO_VISITED  	0x08 	/* This component has been visited */
#define KLINFO_CONTROLLER   	0x10 	/* This component is a device controller */
#define KLINFO_INSTALL   	0x20  	/* Install a driver */
#define	KLINFO_HEADLESS		0x40	/* Headless (or hubless) component */
#define IS_CONSOLE_IOC3(i)	((((klinfo_t *)i)->flags) & KLINFO_INSTALL)

#define GB2		0x80000000

#define MAX_RSV_PTRS	32

/* Structures to manage various data storage areas */

#define BOARD_STRUCT 		0
#define COMPONENT_STRUCT 	1
#define ERRINFO_STRUCT 		2
#define KLMALLOC_TYPE_MAX 	(ERRINFO_STRUCT + 1)
#define DEVICE_STRUCT 		3


typedef struct console_s {
	unsigned long 	uart_base;
	unsigned long 	config_base;
	unsigned long 	memory_base;
	short		baud;
	short		flag;
	int		type;
	nasid_t		nasid;
	char		wid;
	char 		npci;
	nic_t		baseio_nic;
} console_t;

typedef struct klc_malloc_hdr {
        klconf_off_t km_base;
        klconf_off_t km_limit;
        klconf_off_t km_current;
} klc_malloc_hdr_t;

/* Functions/macros needed to use this structure */

typedef struct kl_config_hdr {
	u64		ch_magic;	/* set this to KLCFGINFO_MAGIC */
	u32		ch_version;    /* structure version number */
	klconf_off_t	ch_malloc_hdr_off; /* offset of ch_malloc_hdr */
	klconf_off_t	ch_cons_off;       /* offset of ch_cons */
	klconf_off_t	ch_board_info;	/* the link list of boards */
	console_t	ch_cons_info;	/* address info of the console */
	klc_malloc_hdr_t ch_malloc_hdr[KLMALLOC_TYPE_MAX];
	confidence_t	ch_sw_belief;	/* confidence that software is bad*/
	confidence_t	ch_sn0net_belief; /* confidence that sn0net is bad */
} kl_config_hdr_t;


#define KL_CONFIG_HDR(_nasid) 	((kl_config_hdr_t *)(KLCONFIG_ADDR(_nasid)))
#define KL_CONFIG_INFO_OFFSET(_nasid)					\
        (KL_CONFIG_HDR(_nasid)->ch_board_info)
#define KL_CONFIG_INFO_SET_OFFSET(_nasid, _off)				\
        (KL_CONFIG_HDR(_nasid)->ch_board_info = (_off))

#define KL_CONFIG_INFO(_nasid) 						\
        (lboard_t *)((KL_CONFIG_HDR(_nasid)->ch_board_info) ?		\
	 NODE_OFFSET_TO_K1((_nasid), KL_CONFIG_HDR(_nasid)->ch_board_info) : \
	 0)
#define KL_CONFIG_MAGIC(_nasid)		(KL_CONFIG_HDR(_nasid)->ch_magic)

#define KL_CONFIG_CHECK_MAGIC(_nasid)					\
        (KL_CONFIG_HDR(_nasid)->ch_magic == KLCFGINFO_MAGIC)

#define KL_CONFIG_HDR_INIT_MAGIC(_nasid)	\
                  (KL_CONFIG_HDR(_nasid)->ch_magic = KLCFGINFO_MAGIC)

/* --- New Macros for the changed kl_config_hdr_t structure --- */

#define PTR_CH_MALLOC_HDR(_k)   ((klc_malloc_hdr_t *)\
			((unsigned long)_k + (_k->ch_malloc_hdr_off)))

#define KL_CONFIG_CH_MALLOC_HDR(_n)   PTR_CH_MALLOC_HDR(KL_CONFIG_HDR(_n))

#define PTR_CH_CONS_INFO(_k)	((console_t *)\
			((unsigned long)_k + (_k->ch_cons_off)))

#define KL_CONFIG_CH_CONS_INFO(_n)   PTR_CH_CONS_INFO(KL_CONFIG_HDR(_n))

/* ------------------------------------------------------------- */

#define KL_CONFIG_INFO_START(_nasid)	\
        (klconf_off_t)(KLCONFIG_OFFSET(_nasid) + sizeof(kl_config_hdr_t))

#define KL_CONFIG_BOARD_NASID(_brd)	((_brd)->brd_nasid)
#define KL_CONFIG_BOARD_SET_NEXT(_brd, _off)	((_brd)->brd_next = (_off))

#define KL_CONFIG_DUPLICATE_BOARD(_brd)	((_brd)->brd_flags & DUPLICATE_BOARD)

#define XBOW_PORT_TYPE_HUB(_xbowp, _link) 	\
               ((_xbowp)->xbow_port_info[(_link) - BASE_XBOW_PORT].port_flag & XBOW_PORT_HUB)
#define XBOW_PORT_TYPE_IO(_xbowp, _link) 	\
               ((_xbowp)->xbow_port_info[(_link) - BASE_XBOW_PORT].port_flag & XBOW_PORT_IO)

#define XBOW_PORT_IS_ENABLED(_xbowp, _link) 	\
               ((_xbowp)->xbow_port_info[(_link) - BASE_XBOW_PORT].port_flag & XBOW_PORT_ENABLE)
#define XBOW_PORT_NASID(_xbowp, _link) 	\
               ((_xbowp)->xbow_port_info[(_link) - BASE_XBOW_PORT].port_nasid)

#define XBOW_PORT_IO     0x1
#define XBOW_PORT_HUB    0x2
#define XBOW_PORT_ENABLE 0x4

#define	SN0_PORT_FENCE_SHFT	0
#define	SN0_PORT_FENCE_MASK	(1 << SN0_PORT_FENCE_SHFT)


#define KL_CPU_R4000		0x1	/* Standard R4000 */
#define KL_CPU_TFP		0x2	/* TFP processor */
#define	KL_CPU_R10000		0x3	/* R10000 (T5) */
#define KL_CPU_NONE		(-1)	/* no cpu present in slot */


#define KLCLASS_MASK	0xf0
#define KLCLASS_NONE	0x00
#define KLCLASS_NODE	0x10             /* CPU, Memory and HUB board */
#define KLCLASS_CPU	KLCLASS_NODE
#define KLCLASS_IO	0x20             /* BaseIO, 4 ch SCSI, ethernet, FDDI
					    and the non-graphics widget boards */
#define KLCLASS_ROUTER	0x30             /* Router board */
#define KLCLASS_MIDPLANE 0x40            /* We need to treat this as a board
                                            so that we can record error info */
#define KLCLASS_GFX	0x50		/* graphics boards */

#define KLCLASS_PSEUDO_GFX	0x60	/* HDTV type cards that use a gfx
					 * hw ifc to xtalk and are not gfx
					 * class for sw purposes */

#define KLCLASS_MAX	7		/* Bump this if a new CLASS is added */
#define KLTYPE_MAX	10		/* Bump this if a new CLASS is added */

#define KLCLASS_UNKNOWN	0xf0

#define KLCLASS(_x) ((_x) & KLCLASS_MASK)


#define KLTYPE_MASK	0x0f
#define KLTYPE_NONE	0x00
#define KLTYPE_EMPTY	0x00

#define KLTYPE_WEIRDCPU (KLCLASS_CPU | 0x0)
#define KLTYPE_IP27	(KLCLASS_CPU | 0x1) /* 2 CPUs(R10K) per board */

#define KLTYPE_WEIRDIO	(KLCLASS_IO  | 0x0)
#define KLTYPE_BASEIO	(KLCLASS_IO  | 0x1) /* IOC3, SuperIO, Bridge, SCSI */
#define KLTYPE_IO6	KLTYPE_BASEIO       /* Additional name */
#define KLTYPE_4CHSCSI	(KLCLASS_IO  | 0x2)
#define KLTYPE_MSCSI	KLTYPE_4CHSCSI      /* Additional name */
#define KLTYPE_ETHERNET	(KLCLASS_IO  | 0x3)
#define KLTYPE_MENET	KLTYPE_ETHERNET     /* Additional name */
#define KLTYPE_FDDI  	(KLCLASS_IO  | 0x4)
#define KLTYPE_UNUSED	(KLCLASS_IO  | 0x5) /* XXX UNUSED */
#define KLTYPE_HAROLD   (KLCLASS_IO  | 0x6) /* PCI SHOE BOX */
#define KLTYPE_PCI	KLTYPE_HAROLD
#define KLTYPE_VME      (KLCLASS_IO  | 0x7) /* Any 3rd party VME card */
#define KLTYPE_MIO   	(KLCLASS_IO  | 0x8)
#define KLTYPE_FC    	(KLCLASS_IO  | 0x9)
#define KLTYPE_LINC    	(KLCLASS_IO  | 0xA)
#define KLTYPE_TPU    	(KLCLASS_IO  | 0xB) /* Tensor Processing Unit */
#define KLTYPE_GSN_A   	(KLCLASS_IO  | 0xC) /* Main GSN board */
#define KLTYPE_GSN_B   	(KLCLASS_IO  | 0xD) /* Auxiliary GSN board */

#define KLTYPE_GFX	(KLCLASS_GFX | 0x0) /* unknown graphics type */
#define KLTYPE_GFX_KONA (KLCLASS_GFX | 0x1) /* KONA graphics on IP27 */
#define KLTYPE_GFX_MGRA (KLCLASS_GFX | 0x3) /* MGRAS graphics on IP27 */

#define KLTYPE_WEIRDROUTER (KLCLASS_ROUTER | 0x0)
#define KLTYPE_ROUTER     (KLCLASS_ROUTER | 0x1)
#define KLTYPE_ROUTER2    KLTYPE_ROUTER		/* Obsolete! */
#define KLTYPE_NULL_ROUTER (KLCLASS_ROUTER | 0x2)
#define KLTYPE_META_ROUTER (KLCLASS_ROUTER | 0x3)

#define KLTYPE_WEIRDMIDPLANE (KLCLASS_MIDPLANE | 0x0)
#define KLTYPE_MIDPLANE8  (KLCLASS_MIDPLANE | 0x1) /* 8 slot backplane */
#define KLTYPE_MIDPLANE    KLTYPE_MIDPLANE8
#define KLTYPE_PBRICK_XBOW	(KLCLASS_MIDPLANE | 0x2)

#define KLTYPE_IOBRICK		(KLCLASS_IOBRICK | 0x0)
#define KLTYPE_IBRICK		(KLCLASS_IOBRICK | 0x1)
#define KLTYPE_PBRICK		(KLCLASS_IOBRICK | 0x2)
#define KLTYPE_XBRICK		(KLCLASS_IOBRICK | 0x3)

#define KLTYPE_PBRICK_BRIDGE	KLTYPE_PBRICK

#define KLTYPE_XTHD   	(KLCLASS_PSEUDO_GFX | 0x9)

#define KLTYPE_UNKNOWN	(KLCLASS_UNKNOWN | 0xf)

#define KLTYPE(_x) 	((_x) & KLTYPE_MASK)
#define IS_MIO_PRESENT(l)	((l->brd_type == KLTYPE_BASEIO) && \
				 (l->brd_flags & SECOND_NIC_PRESENT))
#define IS_MIO_IOC3(l, n)	(IS_MIO_PRESENT(l) && (n > 2))


#define MAX_COMPTS_PER_BRD 24

#define LOCAL_BOARD 1
#define REMOTE_BOARD 2

#define LBOARD_STRUCT_VERSION 	2

typedef struct lboard_s {
	klconf_off_t 	brd_next;         /* Next BOARD */
	unsigned char 	struct_type;      /* type of structure, local or remote */
	unsigned char 	brd_type;         /* type+class */
	unsigned char 	brd_sversion;     /* version of this structure */
        unsigned char 	brd_brevision;    /* board revision */
        unsigned char 	brd_promver;      /* board prom version, if any */
	unsigned char 	brd_flags;        /* Enabled, Disabled etc */
	unsigned char 	brd_slot;         /* slot number */
	unsigned short	brd_debugsw;      /* Debug switches */
	moduleid_t	brd_module;       /* module to which it belongs */
	partid_t 	brd_partition;    /* Partition number */
        unsigned short 	brd_diagval;      /* diagnostic value */
        unsigned short 	brd_diagparm;     /* diagnostic parameter */
        unsigned char 	brd_inventory;    /* inventory history */
        unsigned char 	brd_numcompts;    /* Number of components */
        nic_t         	brd_nic;          /* Number in CAN */
	nasid_t		brd_nasid;        /* passed parameter */
	klconf_off_t 	brd_compts[MAX_COMPTS_PER_BRD]; /* pointers to COMPONENTS */
	klconf_off_t 	brd_errinfo;      /* Board's error information */
	struct lboard_s *brd_parent;	  /* Logical parent for this brd */
	vertex_hdl_t	brd_graph_link;   /* vertex hdl to connect extern compts */
	confidence_t	brd_confidence;	  /* confidence that the board is bad */
	nasid_t		brd_owner;        /* who owns this board */
	unsigned char 	brd_nic_flags;    /* To handle 8 more NICs */
	char		brd_name[32];
} lboard_t;



#define BOARD_SLOT(_brd)	((_brd)->brd_slot)

#define KLCF_CLASS(_brd)	KLCLASS((_brd)->brd_type)
#define KLCF_TYPE(_brd)		KLTYPE((_brd)->brd_type)
#define KLCF_REMOTE(_brd)  	(((_brd)->struct_type & LOCAL_BOARD) ? 0 : 1)
#define KLCF_NUM_COMPS(_brd)	((_brd)->brd_numcompts)
#define KLCF_MODULE_ID(_brd)	((_brd)->brd_module)

#define KLCF_NEXT(_brd) 	\
        ((_brd)->brd_next ? 	\
	 (lboard_t *)(NODE_OFFSET_TO_K1(NASID_GET(_brd), (_brd)->brd_next)):\
	 NULL)
#define KLCF_COMP(_brd, _ndx)   \
                (klinfo_t *)(NODE_OFFSET_TO_K1(NASID_GET(_brd),	\
					       (_brd)->brd_compts[(_ndx)]))

#define KLCF_COMP_ERROR(_brd, _comp)	\
               (NODE_OFFSET_TO_K1(NASID_GET(_brd), (_comp)->errinfo))

#define KLCF_COMP_TYPE(_comp)	((_comp)->struct_type)
#define KLCF_BRIDGE_W_ID(_comp)	((_comp)->physid)	/* Widget ID */




typedef struct klinfo_s {                  /* Generic info */
        unsigned char   struct_type;       /* type of this structure */
        unsigned char   struct_version;    /* version of this structure */
        unsigned char   flags;            /* Enabled, disabled etc */
        unsigned char   revision;         /* component revision */
        unsigned short  diagval;          /* result of diagnostics */
        unsigned short  diagparm;         /* diagnostic parameter */
        unsigned char   inventory;        /* previous inventory status */
	nic_t 		nic;              /* MUst be aligned properly */
        unsigned char   physid;           /* physical id of component */
        unsigned int    virtid;           /* virtual id as seen by system */
	unsigned char	widid;	          /* Widget id - if applicable */
	nasid_t		nasid;            /* node number - from parent */
	char		pad1;		  /* pad out structure. */
	char		pad2;		  /* pad out structure. */
	COMPONENT	*arcs_compt;      /* ptr to the arcs struct for ease*/
        klconf_off_t	errinfo;          /* component specific errors */
        unsigned short  pad3;             /* pci fields have moved over to */
        unsigned short  pad4;             /* klbri_t */
} klinfo_t ;

#define KLCONFIG_INFO_ENABLED(_i)	((_i)->flags & KLINFO_ENABLE)
#define KLSTRUCT_UNKNOWN	0
#define KLSTRUCT_CPU  		1
#define KLSTRUCT_HUB  		2
#define KLSTRUCT_MEMBNK 	3
#define KLSTRUCT_XBOW 		4
#define KLSTRUCT_BRI 		5
#define KLSTRUCT_IOC3 		6
#define KLSTRUCT_PCI 		7
#define KLSTRUCT_VME 		8
#define KLSTRUCT_ROU		9
#define KLSTRUCT_GFX 		10
#define KLSTRUCT_SCSI 		11
#define KLSTRUCT_FDDI 		12
#define KLSTRUCT_MIO 		13
#define KLSTRUCT_DISK 		14
#define KLSTRUCT_TAPE 		15
#define KLSTRUCT_CDROM 		16
#define KLSTRUCT_HUB_UART 	17
#define KLSTRUCT_IOC3ENET 	18
#define KLSTRUCT_IOC3UART 	19
#define KLSTRUCT_UNUSED		20 /* XXX UNUSED */
#define KLSTRUCT_IOC3PCKM       21
#define KLSTRUCT_RAD        	22
#define KLSTRUCT_HUB_TTY        23
#define KLSTRUCT_IOC3_TTY 	24


#define KLSTRUCT_FIBERCHANNEL 	25
#define KLSTRUCT_MOD_SERIAL_NUM 26
#define KLSTRUCT_IOC3MS         27
#define KLSTRUCT_TPU            28
#define KLSTRUCT_GSN_A          29
#define KLSTRUCT_GSN_B          30
#define KLSTRUCT_XTHD           31


#define IP27_CPU0_INDEX 0
#define IP27_CPU1_INDEX 1
#define IP27_HUB_INDEX 2
#define IP27_MEM_INDEX 3

#define BASEIO_BRIDGE_INDEX 0
#define BASEIO_IOC3_INDEX 1
#define BASEIO_SCSI1_INDEX 2
#define BASEIO_SCSI2_INDEX 3

#define MIDPLANE_XBOW_INDEX 0
#define ROUTER_COMPONENT_INDEX 0

#define CH4SCSI_BRIDGE_INDEX 0

/* Info holders for various hardware components */

typedef u64 *pci_t;
typedef u64 *vmeb_t;
typedef u64 *vmed_t;
typedef u64 *fddi_t;
typedef u64 *scsi_t;
typedef u64 *mio_t;
typedef u64 *graphics_t;
typedef u64 *router_t;

typedef struct klport_s {
	nasid_t		port_nasid;
	unsigned char	port_flag;
	klconf_off_t	port_offset;
} klport_t;

typedef struct klcpu_s {                          /* CPU */
	klinfo_t 	cpu_info;
	unsigned short 	cpu_prid;	/* Processor PRID value */
	unsigned short 	cpu_fpirr;	/* FPU IRR value */
	unsigned short 	cpu_speed;	/* Speed in MHZ */
	unsigned short 	cpu_scachesz;	/* secondary cache size in MB */
	unsigned short 	cpu_scachespeed;/* secondary cache speed in MHz */
} klcpu_t ;

#define CPU_STRUCT_VERSION   2

typedef struct klhub_s {			/* HUB */
	klinfo_t 	hub_info;
	unsigned int 		hub_flags;		/* PCFG_HUB_xxx flags */
	klport_t	hub_port;		/* hub is connected to this */
	nic_t		hub_box_nic;		/* nic of containing box */
	klconf_off_t	hub_mfg_nic;		/* MFG NIC string */
	u64		hub_speed;		/* Speed of hub in HZ */
} klhub_t ;

typedef struct klhub_uart_s {			/* HUB */
	klinfo_t 	hubuart_info;
	unsigned int 		hubuart_flags;		/* PCFG_HUB_xxx flags */
	nic_t		hubuart_box_nic;	/* nic of containing box */
} klhub_uart_t ;

#define MEMORY_STRUCT_VERSION   2

typedef struct klmembnk_s {			/* MEMORY BANK */
	klinfo_t 	membnk_info;
	short 		membnk_memsz;		/* Total memory in megabytes */
	short		membnk_dimm_select; /* bank to physical addr mapping*/
	short		membnk_bnksz[MD_MEM_BANKS]; /* Memory bank sizes */
	short		membnk_attr;
} klmembnk_t ;

#define KLCONFIG_MEMBNK_SIZE(_info, _bank)	\
                            ((_info)->membnk_bnksz[(_bank)])


#define MEMBNK_PREMIUM 1
#define KLCONFIG_MEMBNK_PREMIUM(_info, _bank)	\
                            ((_info)->membnk_attr & (MEMBNK_PREMIUM << (_bank)))

#define MAX_SERIAL_NUM_SIZE 10

typedef struct klmod_serial_num_s {
      klinfo_t        snum_info;
      union {
              char snum_str[MAX_SERIAL_NUM_SIZE];
              unsigned long long       snum_int;
      } snum;
} klmod_serial_num_t;


#define GET_SNUM_COMP(_l) 	((klmod_serial_num_t *)\
				KLCF_COMP(_l, _l->brd_numcompts))

#define MAX_XBOW_LINKS 16

typedef struct klxbow_s {                          /* XBOW */
	klinfo_t 	xbow_info ;
	klport_t	xbow_port_info[MAX_XBOW_LINKS] ; /* Module number */
        int		xbow_master_hub_link;
        /* type of brd connected+component struct ptr+flags */
} klxbow_t ;

#define MAX_PCI_SLOTS 8

typedef struct klpci_device_s {
	s32	pci_device_id;	/* 32 bits of vendor/device ID. */
	s32	pci_device_pad;	/* 32 bits of padding. */
} klpci_device_t;

#define BRIDGE_STRUCT_VERSION	2

typedef struct klbri_s {                          /* BRIDGE */
	klinfo_t 	bri_info ;
	unsigned char	bri_eprominfo ;    /* IO6prom connected to bridge */
	unsigned char	bri_bustype ;      /* PCI/VME BUS bridge/GIO */
	pci_t    	pci_specific  ;    /* PCI Board config info */
	klpci_device_t	bri_devices[MAX_PCI_DEVS] ;	/* PCI IDs */
	klconf_off_t	bri_mfg_nic ;
} klbri_t ;

#define MAX_IOC3_TTY	2

typedef struct klioc3_s {                          /* IOC3 */
	klinfo_t 	ioc3_info ;
	unsigned char	ioc3_ssram ;        /* Info about ssram */
	unsigned char	ioc3_nvram ;        /* Info about nvram */
	klinfo_t	ioc3_superio ;      /* Info about superio */
	klconf_off_t	ioc3_tty_off ;
	klinfo_t	ioc3_enet ;
	klconf_off_t	ioc3_enet_off ;
	klconf_off_t	ioc3_kbd_off ;
} klioc3_t ;

#define MAX_VME_SLOTS 8

typedef struct klvmeb_s {                          /* VME BRIDGE - PCI CTLR */
	klinfo_t 	vmeb_info ;
	vmeb_t		vmeb_specific ;
	klconf_off_t   	vmeb_brdinfo[MAX_VME_SLOTS]   ;    /* VME Board config info */
} klvmeb_t ;

typedef struct klvmed_s {                          /* VME DEVICE - VME BOARD */
	klinfo_t	vmed_info ;
	vmed_t		vmed_specific ;
	klconf_off_t   	vmed_brdinfo[MAX_VME_SLOTS]   ;    /* VME Board config info */
} klvmed_t ;

#define ROUTER_VECTOR_VERS	2

/* XXX - Don't we need the number of ports here?!? */
typedef struct klrou_s {                          /* ROUTER */
	klinfo_t 	rou_info ;
	unsigned int		rou_flags ;           /* PCFG_ROUTER_xxx flags */
	nic_t		rou_box_nic ;         /* nic of the containing module */
	klport_t 	rou_port[MAX_ROUTER_PORTS + 1] ; /* array index 1 to 6 */
	klconf_off_t	rou_mfg_nic ;     /* MFG NIC string */
	u64	rou_vector;	  /* vector from master node */
} klrou_t ;

#define KLGFX_COOKIE	0x0c0de000

typedef struct klgfx_s {		/* GRAPHICS Device */
	klinfo_t 	gfx_info;
	klconf_off_t    old_gndevs;	/* for compatibility with older proms */
	klconf_off_t    old_gdoff0;	/* for compatibility with older proms */
	unsigned int		cookie;		/* for compatibility with older proms */
	unsigned int		moduleslot;
	struct klgfx_s	*gfx_next_pipe;
	graphics_t	gfx_specific;
	klconf_off_t    pad0;		/* for compatibility with older proms */
	klconf_off_t    gfx_mfg_nic;
} klgfx_t;

typedef struct klxthd_s {
	klinfo_t 	xthd_info ;
	klconf_off_t	xthd_mfg_nic ;        /* MFG NIC string */
} klxthd_t ;

typedef struct kltpu_s {                     /* TPU board */
	klinfo_t 	tpu_info ;
	klconf_off_t	tpu_mfg_nic ;        /* MFG NIC string */
} kltpu_t ;

typedef struct klgsn_s {                     /* GSN board */
	klinfo_t 	gsn_info ;
	klconf_off_t	gsn_mfg_nic ;        /* MFG NIC string */
} klgsn_t ;

#define MAX_SCSI_DEVS 16


typedef struct klscsi_s {                          /* SCSI Controller */
	klinfo_t 	scsi_info ;
	scsi_t       	scsi_specific   ;
	unsigned char 	scsi_numdevs ;
	klconf_off_t	scsi_devinfo[MAX_SCSI_DEVS] ;
} klscsi_t ;

typedef struct klscdev_s {                          /* SCSI device */
	klinfo_t 	scdev_info ;
	struct scsidisk_data *scdev_cfg ; /* driver fills up this */
} klscdev_t ;

typedef struct klttydev_s {                          /* TTY device */
	klinfo_t 	ttydev_info ;
	struct terminal_data *ttydev_cfg ; /* driver fills up this */
} klttydev_t ;

typedef struct klenetdev_s {                          /* ENET device */
	klinfo_t 	enetdev_info ;
	struct net_data *enetdev_cfg ; /* driver fills up this */
} klenetdev_t ;

typedef struct klkbddev_s {                          /* KBD device */
	klinfo_t 	kbddev_info ;
	struct keyboard_data *kbddev_cfg ; /* driver fills up this */
} klkbddev_t ;

typedef struct klmsdev_s {                          /* mouse device */
        klinfo_t        msdev_info ;
        void 		*msdev_cfg ;
} klmsdev_t ;

#define MAX_FDDI_DEVS 10 /* XXX Is this true */

typedef struct klfddi_s {                          /* FDDI */
	klinfo_t 	fddi_info ;
	fddi_t        	fddi_specific ;
	klconf_off_t	fddi_devinfo[MAX_FDDI_DEVS] ;
} klfddi_t ;

typedef struct klmio_s {                          /* MIO */
	klinfo_t 	mio_info ;
	mio_t       	mio_specific   ;
} klmio_t ;


typedef union klcomp_s {
	klcpu_t		kc_cpu;
	klhub_t		kc_hub;
	klmembnk_t 	kc_mem;
	klxbow_t  	kc_xbow;
	klbri_t		kc_bri;
	klioc3_t	kc_ioc3;
	klvmeb_t	kc_vmeb;
	klvmed_t	kc_vmed;
	klrou_t		kc_rou;
	klgfx_t		kc_gfx;
	klscsi_t	kc_scsi;
	klscdev_t	kc_scsi_dev;
	klfddi_t	kc_fddi;
	klmio_t		kc_mio;
	klmod_serial_num_t kc_snum ;
} klcomp_t;

typedef union kldev_s {      /* for device structure allocation */
	klscdev_t	kc_scsi_dev ;
	klttydev_t	kc_tty_dev ;
	klenetdev_t	kc_enet_dev ;
	klkbddev_t 	kc_kbd_dev ;
} kldev_t ;

/* Data structure interface routines. TBD */

/* Include launch info in this file itself? TBD */



typedef union {
	lboard_t *lbinfo ;
} biptr_t ;


#define BRI_PER_XBOW 6
#define PCI_PER_BRI  8
#define DEV_PER_PCI  16


/* Virtual dipswitch values (starting from switch "7"): */

#define VDS_NOGFX		0x8000	/* Don't enable gfx and autoboot */
#define VDS_NOMP		0x100	/* Don't start slave processors */
#define VDS_MANUMODE		0x80	/* Manufacturing mode */
#define VDS_NOARB		0x40	/* No bootmaster arbitration */
#define VDS_PODMODE		0x20	/* Go straight to POD mode */
#define VDS_NO_DIAGS		0x10	/* Don't run any diags after BM arb */
#define VDS_DEFAULTS		0x08	/* Use default environment values */
#define VDS_NOMEMCLEAR		0x04	/* Don't run mem cfg code */
#define VDS_2ND_IO4		0x02	/* Boot from the second IO4 */
#define VDS_DEBUG_PROM		0x01	/* Print PROM debugging messages */

/* external declarations of Linux kernel functions. */

extern lboard_t *find_lboard(lboard_t *start, unsigned char type);
extern klinfo_t *find_component(lboard_t *brd, klinfo_t *kli, unsigned char type);
extern klinfo_t *find_first_component(lboard_t *brd, unsigned char type);
extern klcpu_t *nasid_slice_to_cpuinfo(nasid_t, int);
extern lboard_t *find_lboard_class(lboard_t *start, unsigned char brd_class);


extern klcpu_t *sn_get_cpuinfo(cpuid_t cpu);

#endif /* _ASM_SN_KLCONFIG_H */