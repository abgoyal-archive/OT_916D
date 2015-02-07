

#ifndef __SYSDEP_X86_64_SKAS_PTRACE_H
#define __SYSDEP_X86_64_SKAS_PTRACE_H

struct ptrace_faultinfo {
        int is_write;
        unsigned long addr;
};

struct ptrace_ldt {
        int func;
        void *ptr;
        unsigned long bytecount;
};

#define PTRACE_LDT 54

#endif
