

#ifndef NES_USER_H
#define NES_USER_H

#include <linux/types.h>

#define NES_ABI_USERSPACE_VER 1
#define NES_ABI_KERNEL_VER    1


struct nes_alloc_ucontext_req {
	__u32 reserved32;
	__u8  userspace_ver;
	__u8  reserved8[3];
};

struct nes_alloc_ucontext_resp {
	__u32 max_pds; /* maximum pds allowed for this user process */
	__u32 max_qps; /* maximum qps allowed for this user process */
	__u32 wq_size; /* size of the WQs (sq+rq) allocated to the mmaped area */
	__u8  virtwq;  /* flag to indicate if virtual WQ are to be used or not */
	__u8  kernel_ver;
	__u8  reserved[2];
};

struct nes_alloc_pd_resp {
	__u32 pd_id;
	__u32 mmap_db_index;
};

struct nes_create_cq_req {
	__u64 user_cq_buffer;
	__u32 mcrqf;
	__u8 reserved[4];
};

struct nes_create_qp_req {
	__u64 user_wqe_buffers;
};

enum iwnes_memreg_type {
	IWNES_MEMREG_TYPE_MEM = 0x0000,
	IWNES_MEMREG_TYPE_QP = 0x0001,
	IWNES_MEMREG_TYPE_CQ = 0x0002,
	IWNES_MEMREG_TYPE_MW = 0x0003,
	IWNES_MEMREG_TYPE_FMR = 0x0004,
	IWNES_MEMREG_TYPE_FMEM = 0x0005,
};

struct nes_mem_reg_req {
	__u32 reg_type;	/* indicates if id is memory, QP or CQ */
	__u32 reserved;
};

struct nes_create_cq_resp {
	__u32 cq_id;
	__u32 cq_size;
	__u32 mmap_db_index;
	__u32 reserved;
};

struct nes_create_qp_resp {
	__u32 qp_id;
	__u32 actual_sq_size;
	__u32 actual_rq_size;
	__u32 mmap_sq_db_index;
	__u32 mmap_rq_db_index;
	__u32 nes_drv_opt;
};

#endif				/* NES_USER_H */
