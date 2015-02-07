
#ifndef _ASM_TRAPS_H
#define _ASM_TRAPS_H

#define MIPS_BE_DISCARD	0		/* return with no action */
#define MIPS_BE_FIXUP	1		/* return to the fixup code */
#define MIPS_BE_FATAL	2		/* treat as an unrecoverable error */

extern void (*board_be_init)(void);
extern int (*board_be_handler)(struct pt_regs *regs, int is_fixup);

extern void (*board_nmi_handler_setup)(void);
extern void (*board_ejtag_handler_setup)(void);
extern void (*board_bind_eic_interrupt)(int irq, int regset);

#endif /* _ASM_TRAPS_H */
