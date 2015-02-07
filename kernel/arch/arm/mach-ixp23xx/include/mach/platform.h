

#ifndef __ASSEMBLY__

static inline unsigned long ixp2000_reg_read(volatile void *reg)
{
	return *((volatile unsigned long *)reg);
}

static inline void ixp2000_reg_write(volatile void *reg, unsigned long val)
{
	*((volatile unsigned long *)reg) = val;
}

static inline void ixp2000_reg_wrb(volatile void *reg, unsigned long val)
{
	*((volatile unsigned long *)reg) = val;
}

struct pci_sys_data;

void ixp23xx_map_io(void);
void ixp23xx_init_irq(void);
void ixp23xx_sys_init(void);
int ixp23xx_pci_setup(int, struct pci_sys_data *);
void ixp23xx_pci_preinit(void);
struct pci_bus *ixp23xx_pci_scan_bus(int, struct pci_sys_data*);
void ixp23xx_pci_slave_init(void);

extern struct sys_timer ixp23xx_timer;

#define IXP23XX_UART_XTAL		14745600

#ifndef __ASSEMBLY__
static inline unsigned ixp23xx_cpp_boot(void)
{
	return (*IXP23XX_EXP_CFG0 & IXP23XX_EXP_CFG0_XSI_NOT_PRES);
}
#endif


#endif
