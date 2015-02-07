

#ifndef __MEMORY_DOT_H__
#define __MEMORY_DOT_H__

int dlm_memory_init(void);
void dlm_memory_exit(void);
struct dlm_rsb *dlm_allocate_rsb(struct dlm_ls *ls, int namelen);
void dlm_free_rsb(struct dlm_rsb *r);
struct dlm_lkb *dlm_allocate_lkb(struct dlm_ls *ls);
void dlm_free_lkb(struct dlm_lkb *l);
char *dlm_allocate_lvb(struct dlm_ls *ls);
void dlm_free_lvb(char *l);

#endif		/* __MEMORY_DOT_H__ */

