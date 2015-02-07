



#include <asm/octeon/octeon.h>
#include <asm/octeon/cvmx-helper-jtag.h>


void cvmx_helper_qlm_jtag_init(void)
{
	union cvmx_ciu_qlm_jtgc jtgc;
	uint32_t clock_div = 0;
	uint32_t divisor = cvmx_sysinfo_get()->cpu_clock_hz / (25 * 1000000);
	divisor = (divisor - 1) >> 2;
	/* Convert the divisor into a power of 2 shift */
	while (divisor) {
		clock_div++;
		divisor = divisor >> 1;
	}

	/*
	 * Clock divider for QLM JTAG operations.  eclk is divided by
	 * 2^(CLK_DIV + 2)
	 */
	jtgc.u64 = 0;
	jtgc.s.clk_div = clock_div;
	jtgc.s.mux_sel = 0;
	if (OCTEON_IS_MODEL(OCTEON_CN52XX))
		jtgc.s.bypass = 0x3;
	else
		jtgc.s.bypass = 0xf;
	cvmx_write_csr(CVMX_CIU_QLM_JTGC, jtgc.u64);
	cvmx_read_csr(CVMX_CIU_QLM_JTGC);
}

uint32_t cvmx_helper_qlm_jtag_shift(int qlm, int bits, uint32_t data)
{
	union cvmx_ciu_qlm_jtgd jtgd;
	jtgd.u64 = 0;
	jtgd.s.shift = 1;
	jtgd.s.shft_cnt = bits - 1;
	jtgd.s.shft_reg = data;
	if (!OCTEON_IS_MODEL(OCTEON_CN56XX_PASS1_X))
		jtgd.s.select = 1 << qlm;
	cvmx_write_csr(CVMX_CIU_QLM_JTGD, jtgd.u64);
	do {
		jtgd.u64 = cvmx_read_csr(CVMX_CIU_QLM_JTGD);
	} while (jtgd.s.shift);
	return jtgd.s.shft_reg >> (32 - bits);
}

void cvmx_helper_qlm_jtag_shift_zeros(int qlm, int bits)
{
	while (bits > 0) {
		int n = bits;
		if (n > 32)
			n = 32;
		cvmx_helper_qlm_jtag_shift(qlm, n, 0);
		bits -= n;
	}
}

void cvmx_helper_qlm_jtag_update(int qlm)
{
	union cvmx_ciu_qlm_jtgd jtgd;

	/* Update the new data */
	jtgd.u64 = 0;
	jtgd.s.update = 1;
	if (!OCTEON_IS_MODEL(OCTEON_CN56XX_PASS1_X))
		jtgd.s.select = 1 << qlm;
	cvmx_write_csr(CVMX_CIU_QLM_JTGD, jtgd.u64);
	do {
		jtgd.u64 = cvmx_read_csr(CVMX_CIU_QLM_JTGD);
	} while (jtgd.s.update);
}
