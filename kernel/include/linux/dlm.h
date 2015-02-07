

#ifndef __DLM_DOT_H__
#define __DLM_DOT_H__


/* Lock levels and flags are here */
#include <linux/dlmconstants.h>
#include <linux/types.h>

typedef void dlm_lockspace_t;


#define DLM_SBF_DEMOTED		0x01
#define DLM_SBF_VALNOTVALID	0x02
#define DLM_SBF_ALTMODE		0x04

struct dlm_lksb {
	int 	 sb_status;
	__u32	 sb_lkid;
	char 	 sb_flags;
	char *	 sb_lvbptr;
};

/* dlm_new_lockspace() flags */

#define DLM_LSFL_NODIR		0x00000001
#define DLM_LSFL_TIMEWARN	0x00000002
#define DLM_LSFL_FS     	0x00000004
#define DLM_LSFL_NEWEXCL     	0x00000008

#ifdef __KERNEL__


int dlm_new_lockspace(const char *name, int namelen,
		      dlm_lockspace_t **lockspace, uint32_t flags, int lvblen);


int dlm_release_lockspace(dlm_lockspace_t *lockspace, int force);


int dlm_lock(dlm_lockspace_t *lockspace,
	     int mode,
	     struct dlm_lksb *lksb,
	     uint32_t flags,
	     void *name,
	     unsigned int namelen,
	     uint32_t parent_lkid,
	     void (*lockast) (void *astarg),
	     void *astarg,
	     void (*bast) (void *astarg, int mode));


int dlm_unlock(dlm_lockspace_t *lockspace,
	       uint32_t lkid,
	       uint32_t flags,
	       struct dlm_lksb *lksb,
	       void *astarg);

#endif				/* __KERNEL__ */

#endif				/* __DLM_DOT_H__ */

