

#define S3C6400_PLL_MDIV_MASK	((1 << (25-16+1)) - 1)
#define S3C6400_PLL_PDIV_MASK	((1 << (13-8+1)) - 1)
#define S3C6400_PLL_SDIV_MASK	((1 << (2-0+1)) - 1)
#define S3C6400_PLL_MDIV_SHIFT	(16)
#define S3C6400_PLL_PDIV_SHIFT	(8)
#define S3C6400_PLL_SDIV_SHIFT	(0)

#include <asm/div64.h>
#include <plat/pll6553x.h>

static inline unsigned long s3c6400_get_pll(unsigned long baseclk,
					    u32 pllcon)
{
	u32 mdiv, pdiv, sdiv;
	u64 fvco = baseclk;

	mdiv = (pllcon >> S3C6400_PLL_MDIV_SHIFT) & S3C6400_PLL_MDIV_MASK;
	pdiv = (pllcon >> S3C6400_PLL_PDIV_SHIFT) & S3C6400_PLL_PDIV_MASK;
	sdiv = (pllcon >> S3C6400_PLL_SDIV_SHIFT) & S3C6400_PLL_SDIV_MASK;

	fvco *= mdiv;
	do_div(fvco, (pdiv << sdiv));

	return (unsigned long)fvco;
}

static inline unsigned long s3c6400_get_epll(unsigned long baseclk)
{
	return s3c_get_pll6553x(baseclk, __raw_readl(S3C_EPLL_CON0),
				__raw_readl(S3C_EPLL_CON1));
}
