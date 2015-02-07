

#define AMBA_UART_DR	(*(volatile unsigned char *)0x16000000)
#define AMBA_UART_LCRH	(*(volatile unsigned char *)0x16000008)
#define AMBA_UART_LCRM	(*(volatile unsigned char *)0x1600000c)
#define AMBA_UART_LCRL	(*(volatile unsigned char *)0x16000010)
#define AMBA_UART_CR	(*(volatile unsigned char *)0x16000014)
#define AMBA_UART_FR	(*(volatile unsigned char *)0x16000018)

static void putc(int c)
{
	while (AMBA_UART_FR & (1 << 5))
		barrier();

	AMBA_UART_DR = c;
}

static inline void flush(void)
{
	while (AMBA_UART_FR & (1 << 3))
		barrier();
}

#define arch_decomp_setup()

#define arch_decomp_wdog()
