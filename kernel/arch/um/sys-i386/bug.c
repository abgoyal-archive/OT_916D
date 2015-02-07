

#include <linux/uaccess.h>
#include <asm/errno.h>


int is_valid_bugaddr(unsigned long eip)
{
	unsigned short ud2;

	if (probe_kernel_address((unsigned short __user *)eip, ud2))
		return 0;

	return ud2 == 0x0b0f;
}
