
#ifndef __ASM_GENERIC_POSIX_TYPES_H
#define __ASM_GENERIC_POSIX_TYPES_H

#include <asm/bitsperlong.h>

#ifndef __kernel_ino_t
typedef unsigned long	__kernel_ino_t;
#endif

#ifndef __kernel_mode_t
typedef unsigned int	__kernel_mode_t;
#endif

#ifndef __kernel_nlink_t
typedef unsigned long	__kernel_nlink_t;
#endif

#ifndef __kernel_pid_t
typedef int		__kernel_pid_t;
#endif

#ifndef __kernel_ipc_pid_t
typedef int		__kernel_ipc_pid_t;
#endif

#ifndef __kernel_uid_t
typedef unsigned int	__kernel_uid_t;
typedef unsigned int	__kernel_gid_t;
#endif

#ifndef __kernel_suseconds_t
typedef long		__kernel_suseconds_t;
#endif

#ifndef __kernel_daddr_t
typedef int		__kernel_daddr_t;
#endif

#ifndef __kernel_uid32_t
typedef __kernel_uid_t	__kernel_uid32_t;
typedef __kernel_gid_t	__kernel_gid32_t;
#endif

#ifndef __kernel_old_uid_t
typedef __kernel_uid_t	__kernel_old_uid_t;
typedef __kernel_gid_t	__kernel_old_gid_t;
#endif

#ifndef __kernel_old_dev_t
typedef unsigned int	__kernel_old_dev_t;
#endif

#ifndef __kernel_size_t
#if __BITS_PER_LONG != 64
typedef unsigned int	__kernel_size_t;
typedef int		__kernel_ssize_t;
typedef int		__kernel_ptrdiff_t;
#else
typedef unsigned long	__kernel_size_t;
typedef long		__kernel_ssize_t;
typedef long		__kernel_ptrdiff_t;
#endif
#endif

typedef long		__kernel_off_t;
typedef long long	__kernel_loff_t;
typedef long		__kernel_time_t;
typedef long		__kernel_clock_t;
typedef int		__kernel_timer_t;
typedef int		__kernel_clockid_t;
typedef char *		__kernel_caddr_t;
typedef unsigned short	__kernel_uid16_t;
typedef unsigned short	__kernel_gid16_t;

typedef struct {
	int	val[2];
} __kernel_fsid_t;

#ifdef __KERNEL__

#undef __FD_SET
static inline void __FD_SET(unsigned long __fd, __kernel_fd_set *__fdsetp)
{
	unsigned long __tmp = __fd / __NFDBITS;
	unsigned long __rem = __fd % __NFDBITS;
	__fdsetp->fds_bits[__tmp] |= (1UL<<__rem);
}

#undef __FD_CLR
static inline void __FD_CLR(unsigned long __fd, __kernel_fd_set *__fdsetp)
{
	unsigned long __tmp = __fd / __NFDBITS;
	unsigned long __rem = __fd % __NFDBITS;
	__fdsetp->fds_bits[__tmp] &= ~(1UL<<__rem);
}

#undef __FD_ISSET
static inline int __FD_ISSET(unsigned long __fd, const __kernel_fd_set *__p)
{
	unsigned long __tmp = __fd / __NFDBITS;
	unsigned long __rem = __fd % __NFDBITS;
	return (__p->fds_bits[__tmp] & (1UL<<__rem)) != 0;
}

#undef __FD_ZERO
static inline void __FD_ZERO(__kernel_fd_set *__p)
{
	unsigned long *__tmp = __p->fds_bits;
	int __i;

	if (__builtin_constant_p(__FDSET_LONGS)) {
		switch (__FDSET_LONGS) {
		case 16:
			__tmp[ 0] = 0; __tmp[ 1] = 0;
			__tmp[ 2] = 0; __tmp[ 3] = 0;
			__tmp[ 4] = 0; __tmp[ 5] = 0;
			__tmp[ 6] = 0; __tmp[ 7] = 0;
			__tmp[ 8] = 0; __tmp[ 9] = 0;
			__tmp[10] = 0; __tmp[11] = 0;
			__tmp[12] = 0; __tmp[13] = 0;
			__tmp[14] = 0; __tmp[15] = 0;
			return;

		case 8:
			__tmp[ 0] = 0; __tmp[ 1] = 0;
			__tmp[ 2] = 0; __tmp[ 3] = 0;
			__tmp[ 4] = 0; __tmp[ 5] = 0;
			__tmp[ 6] = 0; __tmp[ 7] = 0;
			return;

		case 4:
			__tmp[ 0] = 0; __tmp[ 1] = 0;
			__tmp[ 2] = 0; __tmp[ 3] = 0;
			return;
		}
	}
	__i = __FDSET_LONGS;
	while (__i) {
		__i--;
		*__tmp = 0;
		__tmp++;
	}
}

#endif /* __KERNEL__ */

#endif /* __ASM_GENERIC_POSIX_TYPES_H */
