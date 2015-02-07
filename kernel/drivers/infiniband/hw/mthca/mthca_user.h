

#ifndef MTHCA_USER_H
#define MTHCA_USER_H

#include <linux/types.h>

#define MTHCA_UVERBS_ABI_VERSION	1


struct mthca_alloc_ucontext_resp {
	__u32 qp_tab_size;
	__u32 uarc_size;
};

struct mthca_alloc_pd_resp {
	__u32 pdn;
	__u32 reserved;
};

struct mthca_reg_mr {
#define MTHCA_MR_DMASYNC	0x1
	__u32 mr_attrs;
	__u32 reserved;
};

struct mthca_create_cq {
	__u32 lkey;
	__u32 pdn;
	__u64 arm_db_page;
	__u64 set_db_page;
	__u32 arm_db_index;
	__u32 set_db_index;
};

struct mthca_create_cq_resp {
	__u32 cqn;
	__u32 reserved;
};

struct mthca_resize_cq {
	__u32 lkey;
	__u32 reserved;
};

struct mthca_create_srq {
	__u32 lkey;
	__u32 db_index;
	__u64 db_page;
};

struct mthca_create_srq_resp {
	__u32 srqn;
	__u32 reserved;
};

struct mthca_create_qp {
	__u32 lkey;
	__u32 reserved;
	__u64 sq_db_page;
	__u64 rq_db_page;
	__u32 sq_db_index;
	__u32 rq_db_index;
};

#endif /* MTHCA_USER_H */
