

#ifndef DTC3280_H
#define DTC3280_H

#define DTCDEBUG 0
#define DTCDEBUG_INIT	0x1
#define DTCDEBUG_TRANSFER 0x2

static int dtc_abort(Scsi_Cmnd *);
static int dtc_biosparam(struct scsi_device *, struct block_device *,
		         sector_t, int*);
static int dtc_detect(struct scsi_host_template *);
static int dtc_queue_command(Scsi_Cmnd *, void (*done)(Scsi_Cmnd *));
static int dtc_bus_reset(Scsi_Cmnd *);

#ifndef CMD_PER_LUN
#define CMD_PER_LUN 2
#endif

#ifndef CAN_QUEUE
#define CAN_QUEUE 32 
#endif

#define NCR5380_implementation_fields \
    void __iomem *base

#define NCR5380_local_declare() \
    void __iomem *base

#define NCR5380_setup(instance) \
    base = ((struct NCR5380_hostdata *)(instance)->hostdata)->base

#define DTC_address(reg) (base + DTC_5380_OFFSET + reg)

#define dbNCR5380_read(reg)                                              \
    (rval=readb(DTC_address(reg)), \
     (((unsigned char) printk("DTC : read register %d at addr %p is: %02x\n"\
    , (reg), DTC_address(reg), rval)), rval ) )

#define dbNCR5380_write(reg, value) do {                                  \
    printk("DTC : write %02x to register %d at address %p\n",         \
            (value), (reg), DTC_address(reg));     \
    writeb(value, DTC_address(reg));} while(0)


#if !(DTCDEBUG & DTCDEBUG_TRANSFER) 
#define NCR5380_read(reg) (readb(DTC_address(reg)))
#define NCR5380_write(reg, value) (writeb(value, DTC_address(reg)))
#else
#define NCR5380_read(reg) (readb(DTC_address(reg)))
#define xNCR5380_read(reg)						\
    (((unsigned char) printk("DTC : read register %d at address %p\n"\
    , (reg), DTC_address(reg))), readb(DTC_address(reg)))

#define NCR5380_write(reg, value) do {					\
    printk("DTC : write %02x to register %d at address %p\n", 	\
	    (value), (reg), DTC_address(reg));	\
    writeb(value, DTC_address(reg));} while(0)
#endif

#define NCR5380_intr			dtc_intr
#define NCR5380_queue_command		dtc_queue_command
#define NCR5380_abort			dtc_abort
#define NCR5380_bus_reset		dtc_bus_reset
#define NCR5380_proc_info		dtc_proc_info 


#define DTC_IRQS 0x9c00


#endif /* DTC3280_H */
