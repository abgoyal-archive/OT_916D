

#include <linux/kexec.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <asm/cacheflush.h>
#include <asm/hw_irq.h>
#include <asm/io.h>

typedef NORET_TYPE void (*relocate_new_kernel_t)(
				unsigned long indirection_page,
				unsigned long reboot_code_buffer,
				unsigned long start_address) ATTRIB_NORET;

void default_machine_kexec(struct kimage *image)
{
	extern const unsigned char relocate_new_kernel[];
	extern const unsigned int relocate_new_kernel_size;
	unsigned long page_list;
	unsigned long reboot_code_buffer, reboot_code_buffer_phys;
	relocate_new_kernel_t rnk;

	/* Interrupts aren't acceptable while we reboot */
	local_irq_disable();

	page_list = image->head;

	/* we need both effective and real address here */
	reboot_code_buffer =
			(unsigned long)page_address(image->control_code_page);
	reboot_code_buffer_phys = virt_to_phys((void *)reboot_code_buffer);

	/* copy our kernel relocation code to the control code page */
	memcpy((void *)reboot_code_buffer, relocate_new_kernel,
						relocate_new_kernel_size);

	flush_icache_range(reboot_code_buffer,
				reboot_code_buffer + KEXEC_CONTROL_PAGE_SIZE);
	printk(KERN_INFO "Bye!\n");

	/* now call it */
	rnk = (relocate_new_kernel_t) reboot_code_buffer;
	(*rnk)(page_list, reboot_code_buffer_phys, image->start);
}

int default_machine_kexec_prepare(struct kimage *image)
{
	return 0;
}
