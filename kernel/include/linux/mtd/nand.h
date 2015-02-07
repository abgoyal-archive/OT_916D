
#ifndef __LINUX_MTD_NAND_H
#define __LINUX_MTD_NAND_H

#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/flashchip.h>
#include <linux/mtd/bbm.h>

struct mtd_info;
struct nand_flash_dev;
/* Scan and identify a NAND device */
extern int nand_scan (struct mtd_info *mtd, int max_chips);
extern int nand_scan_ident(struct mtd_info *mtd, int max_chips,
			   struct nand_flash_dev *table);
extern int nand_scan_tail(struct mtd_info *mtd);

/* Free resources held by the NAND device */
extern void nand_release (struct mtd_info *mtd);

/* Internal helper for board drivers which need to override command function */
extern void nand_wait_ready(struct mtd_info *mtd);

/* locks all blockes present in the device */
extern int nand_lock(struct mtd_info *mtd, loff_t ofs, uint64_t len);

/* unlocks specified locked blockes */
extern int nand_unlock(struct mtd_info *mtd, loff_t ofs, uint64_t len);

/* The maximum number of NAND chips in an array */
#define NAND_MAX_CHIPS		8

#define NAND_MAX_OOBSIZE	256
#define NAND_MAX_PAGESIZE	4096

/* Select the chip by setting nCE to low */
#define NAND_NCE		0x01
/* Select the command latch by setting CLE to high */
#define NAND_CLE		0x02
/* Select the address latch by setting ALE to high */
#define NAND_ALE		0x04

#define NAND_CTRL_CLE		(NAND_NCE | NAND_CLE)
#define NAND_CTRL_ALE		(NAND_NCE | NAND_ALE)
#define NAND_CTRL_CHANGE	0x80

#define NAND_CMD_READ0		0
#define NAND_CMD_READ1		1
#define NAND_CMD_RNDOUT		5
#define NAND_CMD_PAGEPROG	0x10
#define NAND_CMD_READOOB	0x50
#define NAND_CMD_ERASE1		0x60
#define NAND_CMD_STATUS		0x70
#define NAND_CMD_STATUS_MULTI	0x71
#define NAND_CMD_SEQIN		0x80
#define NAND_CMD_RNDIN		0x85
#define NAND_CMD_READID		0x90
#define NAND_CMD_ERASE2		0xd0
#define NAND_CMD_RESET		0xff

#define NAND_CMD_LOCK		0x2a
#define NAND_CMD_UNLOCK1	0x23
#define NAND_CMD_UNLOCK2	0x24

/* Extended commands for large page devices */
#define NAND_CMD_READSTART	0x30
#define NAND_CMD_RNDOUTSTART	0xE0
#define NAND_CMD_CACHEDPROG	0x15

/* Extended commands for AG-AND device */
#define NAND_CMD_DEPLETE1	0x100
#define NAND_CMD_DEPLETE2	0x38
#define NAND_CMD_STATUS_MULTI	0x71
#define NAND_CMD_STATUS_ERROR	0x72
/* multi-bank error status (banks 0-3) */
#define NAND_CMD_STATUS_ERROR0	0x73
#define NAND_CMD_STATUS_ERROR1	0x74
#define NAND_CMD_STATUS_ERROR2	0x75
#define NAND_CMD_STATUS_ERROR3	0x76
#define NAND_CMD_STATUS_RESET	0x7f
#define NAND_CMD_STATUS_CLEAR	0xff

#define NAND_CMD_NONE		-1

/* Status bits */
#define NAND_STATUS_FAIL	0x01
#define NAND_STATUS_FAIL_N1	0x02
#define NAND_STATUS_TRUE_READY	0x20
#define NAND_STATUS_READY	0x40
#define NAND_STATUS_WP		0x80

typedef enum {
	NAND_ECC_NONE,
	NAND_ECC_SOFT,
	NAND_ECC_HW,
	NAND_ECC_HW_SYNDROME,
	NAND_ECC_HW_OOB_FIRST,
} nand_ecc_modes_t;

/* Reset Hardware ECC for read */
#define NAND_ECC_READ		0
/* Reset Hardware ECC for write */
#define NAND_ECC_WRITE		1
/* Enable Hardware ECC before syndrom is read back from flash */
#define NAND_ECC_READSYN	2

/* Bit mask for flags passed to do_nand_read_ecc */
#define NAND_GET_DEVICE		0x80


/* Chip can not auto increment pages */
#define NAND_NO_AUTOINCR	0x00000001
/* Buswitdh is 16 bit */
#define NAND_BUSWIDTH_16	0x00000002
/* Device supports partial programming without padding */
#define NAND_NO_PADDING		0x00000004
/* Chip has cache program function */
#define NAND_CACHEPRG		0x00000008
/* Chip has copy back function */
#define NAND_COPYBACK		0x00000010
#define NAND_IS_AND		0x00000020
#define NAND_4PAGE_ARRAY	0x00000040
#define BBT_AUTO_REFRESH	0x00000080
#define NAND_NO_READRDY		0x00000100
/* Chip does not allow subpage writes */
#define NAND_NO_SUBPAGE_WRITE	0x00000200
/* Chip stores bad block marker on the last page of the eraseblock */
#define NAND_BB_LAST_PAGE	0x00000400

/* Device is one of 'new' xD cards that expose fake nand command set */
#define NAND_BROKEN_XD		0x00000400

/* Device behaves just like nand, but is readonly */
#define NAND_ROM		0x00000800

/* Options valid for Samsung large page devices */
#define NAND_SAMSUNG_LP_OPTIONS \
	(NAND_NO_PADDING | NAND_CACHEPRG | NAND_COPYBACK)

/* Macros to identify the above */
#define NAND_CANAUTOINCR(chip) (!(chip->options & NAND_NO_AUTOINCR))
#define NAND_MUST_PAD(chip) (!(chip->options & NAND_NO_PADDING))
#define NAND_HAS_CACHEPROG(chip) ((chip->options & NAND_CACHEPRG))
#define NAND_HAS_COPYBACK(chip) ((chip->options & NAND_COPYBACK))
/* Large page NAND with SOFT_ECC should support subpage reads */
#define NAND_SUBPAGE_READ(chip) ((chip->ecc.mode == NAND_ECC_SOFT) \
					&& (chip->page_shift > 9))

/* Mask to zero out the chip options, which come from the id table */
#define NAND_CHIPOPTIONS_MSK	(0x0000ffff & ~NAND_NO_AUTOINCR)

/* Non chip related options */
#define NAND_USE_FLASH_BBT	0x00010000
/* This option skips the bbt scan during initialization. */
#define NAND_SKIP_BBTSCAN	0x00020000
#define NAND_OWN_BUFFERS	0x00040000
/* Chip may not exist, so silence any errors in scan */
#define NAND_SCAN_SILENT_NODEV	0x00080000

/* Options set by nand scan */
/* Nand scan has allocated controller struct */
#define NAND_CONTROLLER_ALLOC	0x80000000

/* Cell info constants */
#define NAND_CI_CHIPNR_MSK	0x03
#define NAND_CI_CELLTYPE_MSK	0x0C

/* Keep gcc happy */
struct nand_chip;

struct nand_hw_control {
	spinlock_t	 lock;
	struct nand_chip *active;
	wait_queue_head_t wq;
};

struct nand_ecc_ctrl {
	nand_ecc_modes_t	mode;
	int			steps;
	int			size;
	int			bytes;
	int			total;
	int			prepad;
	int			postpad;
	struct nand_ecclayout	*layout;
	void			(*hwctl)(struct mtd_info *mtd, int mode);
	int			(*calculate)(struct mtd_info *mtd,
					     const uint8_t *dat,
					     uint8_t *ecc_code);
	int			(*correct)(struct mtd_info *mtd, uint8_t *dat,
					   uint8_t *read_ecc,
					   uint8_t *calc_ecc);
	int			(*read_page_raw)(struct mtd_info *mtd,
						 struct nand_chip *chip,
						 uint8_t *buf, int page);
	void			(*write_page_raw)(struct mtd_info *mtd,
						  struct nand_chip *chip,
						  const uint8_t *buf);
	int			(*read_page)(struct mtd_info *mtd,
					     struct nand_chip *chip,
					     uint8_t *buf, int page);
	int			(*read_subpage)(struct mtd_info *mtd,
					     struct nand_chip *chip,
					     uint32_t offs, uint32_t len,
					     uint8_t *buf);
	void			(*write_page)(struct mtd_info *mtd,
					      struct nand_chip *chip,
					      const uint8_t *buf);
	int			(*read_oob)(struct mtd_info *mtd,
					    struct nand_chip *chip,
					    int page,
					    int sndcmd);
	int			(*write_oob)(struct mtd_info *mtd,
					     struct nand_chip *chip,
					     int page);
};

struct nand_buffers {
	uint8_t	ecccalc[NAND_MAX_OOBSIZE];
	uint8_t	ecccode[NAND_MAX_OOBSIZE];
	uint8_t databuf[NAND_MAX_PAGESIZE + NAND_MAX_OOBSIZE];
};


struct nand_chip {
	void  __iomem	*IO_ADDR_R;
	void  __iomem	*IO_ADDR_W;

	uint8_t		(*read_byte)(struct mtd_info *mtd);
	u16		(*read_word)(struct mtd_info *mtd);
	void		(*write_buf)(struct mtd_info *mtd, const uint8_t *buf, int len);
	void		(*read_buf)(struct mtd_info *mtd, uint8_t *buf, int len);
	int		(*verify_buf)(struct mtd_info *mtd, const uint8_t *buf, int len);
	void		(*select_chip)(struct mtd_info *mtd, int chip);
	int		(*block_bad)(struct mtd_info *mtd, loff_t ofs, int getchip);
	int		(*block_markbad)(struct mtd_info *mtd, loff_t ofs);
	void		(*cmd_ctrl)(struct mtd_info *mtd, int dat,
				    unsigned int ctrl);
	int		(*dev_ready)(struct mtd_info *mtd);
	void		(*cmdfunc)(struct mtd_info *mtd, unsigned command, int column, int page_addr);
	int		(*waitfunc)(struct mtd_info *mtd, struct nand_chip *this);
	void		(*erase_cmd)(struct mtd_info *mtd, int page);
	int		(*scan_bbt)(struct mtd_info *mtd);
	int		(*errstat)(struct mtd_info *mtd, struct nand_chip *this, int state, int status, int page);
	int		(*write_page)(struct mtd_info *mtd, struct nand_chip *chip,
				      const uint8_t *buf, int page, int cached, int raw);
#if (CONFIG_MTK_MTD_NAND)
    int     (*read_page)(struct mtd_info *mtd, struct nand_chip *chip, u8 *buf, int page);
    int     (*erase)(struct mtd_info *mtd, int page);
#endif

	int		chip_delay;
	unsigned int	options;

	int		page_shift;
	int		phys_erase_shift;
	int		bbt_erase_shift;
	int		chip_shift;
	int		numchips;
	uint64_t	chipsize;
	int		pagemask;
	int		pagebuf;
	int		subpagesize;
	uint8_t		cellinfo;
	int		badblockpos;
	int		badblockbits;

	flstate_t	state;

	uint8_t		*oob_poi;
	struct nand_hw_control  *controller;
	struct nand_ecclayout	*ecclayout;

	struct nand_ecc_ctrl ecc;
	struct nand_buffers *buffers;
	struct nand_hw_control hwcontrol;

	struct mtd_oob_ops ops;

	uint8_t		*bbt;
	struct nand_bbt_descr	*bbt_td;
	struct nand_bbt_descr	*bbt_md;

	struct nand_bbt_descr	*badblock_pattern;

	void		*priv;
};

#define NAND_MFR_TOSHIBA	0x98
#define NAND_MFR_SAMSUNG	0xec
#define NAND_MFR_FUJITSU	0x04
#define NAND_MFR_NATIONAL	0x8f
#define NAND_MFR_RENESAS	0x07
#define NAND_MFR_STMICRO	0x20
#define NAND_MFR_HYNIX		0xad
#define NAND_MFR_MICRON		0x2c
#define NAND_MFR_AMD		0x01

struct nand_flash_dev {
	char *name;
	int id;
	unsigned long pagesize;
	unsigned long chipsize;
	unsigned long erasesize;
	unsigned long options;
};

struct nand_manufacturers {
	int id;
	char * name;
};

extern struct nand_flash_dev nand_flash_ids[];
extern struct nand_manufacturers nand_manuf_ids[];

extern int nand_scan_bbt(struct mtd_info *mtd, struct nand_bbt_descr *bd);
extern int nand_update_bbt(struct mtd_info *mtd, loff_t offs);
extern int nand_default_bbt(struct mtd_info *mtd);
extern int nand_isbad_bbt(struct mtd_info *mtd, loff_t offs, int allowbbt);
extern int nand_erase_nand(struct mtd_info *mtd, struct erase_info *instr,
			   int allowbbt);
extern int nand_do_read(struct mtd_info *mtd, loff_t from, size_t len,
			size_t * retlen, uint8_t * buf);

struct platform_nand_chip {
	int			nr_chips;
	int			chip_offset;
	int			nr_partitions;
	struct mtd_partition	*partitions;
	struct nand_ecclayout	*ecclayout;
	int			chip_delay;
	unsigned int		options;
	const char		**part_probe_types;
	void			(*set_parts)(uint64_t size,
					struct platform_nand_chip *chip);
	void			*priv;
};

/* Keep gcc happy */
struct platform_device;

struct platform_nand_ctrl {
	int		(*probe)(struct platform_device *pdev);
	void		(*remove)(struct platform_device *pdev);
	void		(*hwcontrol)(struct mtd_info *mtd, int cmd);
	int		(*dev_ready)(struct mtd_info *mtd);
	void		(*select_chip)(struct mtd_info *mtd, int chip);
	void		(*cmd_ctrl)(struct mtd_info *mtd, int dat,
				    unsigned int ctrl);
	void		(*write_buf)(struct mtd_info *mtd,
				    const uint8_t *buf, int len);
	void		(*read_buf)(struct mtd_info *mtd,
				    uint8_t *buf, int len);
	void		*priv;
};

struct platform_nand_data {
	struct platform_nand_chip	chip;
	struct platform_nand_ctrl	ctrl;
};

/* Some helpers to access the data structures */
static inline
struct platform_nand_chip *get_platform_nandchip(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;

	return chip->priv;
}

#endif /* __LINUX_MTD_NAND_H */
