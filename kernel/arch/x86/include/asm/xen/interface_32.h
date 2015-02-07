

#ifndef _ASM_X86_XEN_INTERFACE_32_H
#define _ASM_X86_XEN_INTERFACE_32_H


#define FLAT_RING1_CS 0xe019    /* GDT index 259 */
#define FLAT_RING1_DS 0xe021    /* GDT index 260 */
#define FLAT_RING1_SS 0xe021    /* GDT index 260 */
#define FLAT_RING3_CS 0xe02b    /* GDT index 261 */
#define FLAT_RING3_DS 0xe033    /* GDT index 262 */
#define FLAT_RING3_SS 0xe033    /* GDT index 262 */

#define FLAT_KERNEL_CS FLAT_RING1_CS
#define FLAT_KERNEL_DS FLAT_RING1_DS
#define FLAT_KERNEL_SS FLAT_RING1_SS
#define FLAT_USER_CS    FLAT_RING3_CS
#define FLAT_USER_DS    FLAT_RING3_DS
#define FLAT_USER_SS    FLAT_RING3_SS

/* And the trap vector is... */
#define TRAP_INSTR "int $0x82"

#define __HYPERVISOR_VIRT_START 0xF5800000

#ifndef __ASSEMBLY__

struct cpu_user_regs {
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
    uint32_t ebp;
    uint32_t eax;
    uint16_t error_code;    /* private */
    uint16_t entry_vector;  /* private */
    uint32_t eip;
    uint16_t cs;
    uint8_t  saved_upcall_mask;
    uint8_t  _pad0;
    uint32_t eflags;        /* eflags.IF == !saved_upcall_mask */
    uint32_t esp;
    uint16_t ss, _pad1;
    uint16_t es, _pad2;
    uint16_t ds, _pad3;
    uint16_t fs, _pad4;
    uint16_t gs, _pad5;
};
DEFINE_GUEST_HANDLE_STRUCT(cpu_user_regs);

typedef uint64_t tsc_timestamp_t; /* RDTSC timestamp */

struct arch_vcpu_info {
    unsigned long cr2;
    unsigned long pad[5]; /* sizeof(struct vcpu_info) == 64 */
};

struct xen_callback {
	unsigned long cs;
	unsigned long eip;
};
typedef struct xen_callback xen_callback_t;

#define XEN_CALLBACK(__cs, __eip)				\
	((struct xen_callback){ .cs = (__cs), .eip = (unsigned long)(__eip) })
#endif /* !__ASSEMBLY__ */


#define xen_pfn_to_cr3(pfn) (((unsigned)(pfn) << 12) | ((unsigned)(pfn) >> 20))
#define xen_cr3_to_pfn(cr3) (((unsigned)(cr3) >> 12) | ((unsigned)(cr3) << 20))

#endif /* _ASM_X86_XEN_INTERFACE_32_H */
