

// Includes

#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <asm/io.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>

// Defines

// - Hardware specific

#define WINDOW_START 0xdc000

/* Number of bits in offset. */
#define WINDOW_SHIFT 14
#define WINDOW_LENGTH (1 << WINDOW_SHIFT)

/* The bits for the offset into the window. */
#define WINDOW_MASK (WINDOW_LENGTH-1)
#define PAGE_IO 0x258
#define PAGE_IO_SIZE 2

/* bit 7 of 0x259 must be 1 to enable device. */
#define DEVICE_ENABLE 0x8000

// - Flash / Partition sizing

#define MAX_SIZE_KiB             16384
#define BOOT_PARTITION_SIZE_KiB  768
#define DATA_PARTITION_SIZE_KiB  1280
#define APP_PARTITION_SIZE_KiB   6144

// Globals

static volatile int page_in_window = -1; // Current page in window.
static void __iomem *iomapadr;
static DEFINE_SPINLOCK(sbc_gxx_spin);

static struct mtd_partition partition_info[]={
    { .name = "SBC-GXx flash boot partition",
      .offset = 0,
      .size =   BOOT_PARTITION_SIZE_KiB*1024 },
    { .name = "SBC-GXx flash data partition",
      .offset = BOOT_PARTITION_SIZE_KiB*1024,
      .size = (DATA_PARTITION_SIZE_KiB)*1024 },
    { .name = "SBC-GXx flash application partition",
      .offset = (BOOT_PARTITION_SIZE_KiB+DATA_PARTITION_SIZE_KiB)*1024 }
};

#define NUM_PARTITIONS 3

static inline void sbc_gxx_page(struct map_info *map, unsigned long ofs)
{
	unsigned long page = ofs >> WINDOW_SHIFT;

	if( page!=page_in_window ) {
		outw( page | DEVICE_ENABLE, PAGE_IO );
		page_in_window = page;
	}
}


static map_word sbc_gxx_read8(struct map_info *map, unsigned long ofs)
{
	map_word ret;
	spin_lock(&sbc_gxx_spin);
	sbc_gxx_page(map, ofs);
	ret.x[0] = readb(iomapadr + (ofs & WINDOW_MASK));
	spin_unlock(&sbc_gxx_spin);
	return ret;
}

static void sbc_gxx_copy_from(struct map_info *map, void *to, unsigned long from, ssize_t len)
{
	while(len) {
		unsigned long thislen = len;
		if (len > (WINDOW_LENGTH - (from & WINDOW_MASK)))
			thislen = WINDOW_LENGTH-(from & WINDOW_MASK);

		spin_lock(&sbc_gxx_spin);
		sbc_gxx_page(map, from);
		memcpy_fromio(to, iomapadr + (from & WINDOW_MASK), thislen);
		spin_unlock(&sbc_gxx_spin);
		to += thislen;
		from += thislen;
		len -= thislen;
	}
}

static void sbc_gxx_write8(struct map_info *map, map_word d, unsigned long adr)
{
	spin_lock(&sbc_gxx_spin);
	sbc_gxx_page(map, adr);
	writeb(d.x[0], iomapadr + (adr & WINDOW_MASK));
	spin_unlock(&sbc_gxx_spin);
}

static void sbc_gxx_copy_to(struct map_info *map, unsigned long to, const void *from, ssize_t len)
{
	while(len) {
		unsigned long thislen = len;
		if (len > (WINDOW_LENGTH - (to & WINDOW_MASK)))
			thislen = WINDOW_LENGTH-(to & WINDOW_MASK);

		spin_lock(&sbc_gxx_spin);
		sbc_gxx_page(map, to);
		memcpy_toio(iomapadr + (to & WINDOW_MASK), from, thislen);
		spin_unlock(&sbc_gxx_spin);
		to += thislen;
		from += thislen;
		len -= thislen;
	}
}

static struct map_info sbc_gxx_map = {
	.name = "SBC-GXx flash",
	.phys = NO_XIP,
	.size = MAX_SIZE_KiB*1024, /* this must be set to a maximum possible amount
			 of flash so the cfi probe routines find all
			 the chips */
	.bankwidth = 1,
	.read = sbc_gxx_read8,
	.copy_from = sbc_gxx_copy_from,
	.write = sbc_gxx_write8,
	.copy_to = sbc_gxx_copy_to
};

/* MTD device for all of the flash. */
static struct mtd_info *all_mtd;

static void cleanup_sbc_gxx(void)
{
	if( all_mtd ) {
		del_mtd_partitions( all_mtd );
		map_destroy( all_mtd );
	}

	iounmap(iomapadr);
	release_region(PAGE_IO,PAGE_IO_SIZE);
}

static int __init init_sbc_gxx(void)
{
  	iomapadr = ioremap(WINDOW_START, WINDOW_LENGTH);
	if (!iomapadr) {
		printk( KERN_ERR"%s: failed to ioremap memory region\n",
			sbc_gxx_map.name );
		return -EIO;
	}

	if (!request_region( PAGE_IO, PAGE_IO_SIZE, "SBC-GXx flash")) {
		printk( KERN_ERR"%s: IO ports 0x%x-0x%x in use\n",
			sbc_gxx_map.name,
			PAGE_IO, PAGE_IO+PAGE_IO_SIZE-1 );
		iounmap(iomapadr);
		return -EAGAIN;
	}


	printk( KERN_INFO"%s: IO:0x%x-0x%x MEM:0x%x-0x%x\n",
		sbc_gxx_map.name,
		PAGE_IO, PAGE_IO+PAGE_IO_SIZE-1,
		WINDOW_START, WINDOW_START+WINDOW_LENGTH-1 );

	/* Probe for chip. */
	all_mtd = do_map_probe( "cfi_probe", &sbc_gxx_map );
	if( !all_mtd ) {
		cleanup_sbc_gxx();
		return -ENXIO;
	}

	all_mtd->owner = THIS_MODULE;

	/* Create MTD devices for each partition. */
	add_mtd_partitions(all_mtd, partition_info, NUM_PARTITIONS );

	return 0;
}

module_init(init_sbc_gxx);
module_exit(cleanup_sbc_gxx);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Arcom Control Systems Ltd.");
MODULE_DESCRIPTION("MTD map driver for SBC-GXm and SBC-GX1 series boards");
