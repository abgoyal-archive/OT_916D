
#ifndef __XFS_SUPPORT_UUID_H__
#define __XFS_SUPPORT_UUID_H__

typedef struct {
	unsigned char	__u_bits[16];
} uuid_t;

extern int uuid_is_nil(uuid_t *uuid);
extern int uuid_equal(uuid_t *uuid1, uuid_t *uuid2);
extern void uuid_getnodeuniq(uuid_t *uuid, int fsid [2]);

#endif	/* __XFS_SUPPORT_UUID_H__ */
