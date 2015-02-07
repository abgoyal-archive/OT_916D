

#include <linux/serial.h>
#include <linux/serial_core.h>
#include <linux/serial_reg.h>

#include <asm/bootinfo.h>
#include <asm/io.h>
#include <asm/processor.h>
#include <asm/serial.h>
#include <linux/serial_8250.h>

#include <msp_prom.h>
#include <msp_int.h>
#include <msp_regs.h>

void __init msp_serial_setup(void)
{
	char    *s;
	char    *endp;
	struct uart_port up;
	unsigned int uartclk;

	memset(&up, 0, sizeof(up));

	/* Check if clock was specified in environment */
	s = prom_getenv("uartfreqhz");
	if(!(s && *s && (uartclk = simple_strtoul(s, &endp, 10)) && *endp == 0))
		uartclk = MSP_BASE_BAUD;
	ppfinit("UART clock set to %d\n", uartclk);

	/* Initialize first serial port */
	up.mapbase      = MSP_UART0_BASE;
	up.membase      = ioremap_nocache(up.mapbase, MSP_UART_REG_LEN);
	up.irq          = MSP_INT_UART0;
	up.uartclk      = uartclk;
	up.regshift     = 2;
	up.iotype       = UPIO_DWAPB; /* UPIO_MEM like */
	up.flags        = ASYNC_BOOT_AUTOCONF | ASYNC_SKIP_TEST;
	up.type         = PORT_16550A;
	up.line         = 0;
	up.private_data		= (void*)UART0_STATUS_REG;
	if (early_serial_setup(&up))
		printk(KERN_ERR "Early serial init of port 0 failed\n");

	/* Initialize the second serial port, if one exists */
	switch (mips_machtype) {
		case MACH_MSP4200_EVAL:
		case MACH_MSP4200_GW:
		case MACH_MSP4200_FPGA:
		case MACH_MSP7120_EVAL:
		case MACH_MSP7120_GW:
		case MACH_MSP7120_FPGA:
			/* Enable UART1 on MSP4200 and MSP7120 */
			*GPIO_CFG2_REG = 0x00002299;
			break;

		default:
			return; /* No second serial port, good-bye. */
	}

	up.mapbase      = MSP_UART1_BASE;
	up.membase      = ioremap_nocache(up.mapbase, MSP_UART_REG_LEN);
	up.irq          = MSP_INT_UART1;
	up.line         = 1;
	up.private_data		= (void*)UART1_STATUS_REG;
	if (early_serial_setup(&up))
		printk(KERN_ERR "Early serial init of port 1 failed\n");
}
