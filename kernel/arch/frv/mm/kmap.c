

#include <linux/mm.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/vmalloc.h>

#include <asm/setup.h>
#include <asm/segment.h>
#include <asm/page.h>
#include <asm/pgalloc.h>
#include <asm/io.h>
#include <asm/system.h>

#undef DEBUG

/*****************************************************************************/

void __iomem *__ioremap(unsigned long physaddr, unsigned long size, int cacheflag)
{
	return (void __iomem *)physaddr;
}

void iounmap(void volatile __iomem *addr)
{
}

void kernel_set_cachemode(void *addr, unsigned long size, int cmode)
{
}
