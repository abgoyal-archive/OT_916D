
#ifndef _ASM_USER_H
#define _ASM_USER_H

#include <asm/page.h>
#include <asm/registers.h>


struct user {
	/* We start with the registers, to mimic the way that "memory" is returned
	 * from the ptrace(3,...) function.  */
	struct user_context	regs;

	/* The rest of this junk is to help gdb figure out what goes where */
	unsigned long		u_tsize;	/* Text segment size (pages). */
	unsigned long		u_dsize;	/* Data segment size (pages). */
	unsigned long		u_ssize;	/* Stack segment size (pages). */
	unsigned long		start_code;     /* Starting virtual address of text. */
	unsigned long		start_stack;	/* Starting virtual address of stack area.
						 * This is actually the bottom of the stack,
						 * the top of the stack is always found in the
						 * esp register.  */
	long int		signal;		/* Signal that caused the core dump. */

	unsigned long		magic;		/* To uniquely identify a core file */
	char			u_comm[32];	/* User command that was responsible */
};

#define NBPG			PAGE_SIZE
#define UPAGES			1
#define HOST_TEXT_START_ADDR	(u.start_code)
#define HOST_STACK_END_ADDR	(u.start_stack + u.u_ssize * NBPG)

#endif
