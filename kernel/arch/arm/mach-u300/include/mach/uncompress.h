
#define AMBA_UART_DR	(*(volatile unsigned char *)0xc0013000)
#define AMBA_UART_LCRH	(*(volatile unsigned char *)0xc001302C)
#define AMBA_UART_CR	(*(volatile unsigned char *)0xc0013030)
#define AMBA_UART_FR	(*(volatile unsigned char *)0xc0013018)

static inline void putc(int c)
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
