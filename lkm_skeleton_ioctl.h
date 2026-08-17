// Shared by lkm_skeleton.c (kernel) and lkmctl.c/test_ioctl.c (userspace), so both
// sides agree on the same numeric ioctl() command values.

#ifndef LKM_SKELETON_IOCTL_H
#define LKM_SKELETON_IOCTL_H

#include <linux/ioctl.h>

#define LKM_IOC_MAGIC 'k'

// _IO/_IOR pack {magic, sequence, direction, size} into one unique command integer.
#define LKM_IOC_RESET    _IO(LKM_IOC_MAGIC, 1)
#define LKM_IOC_GET_SIZE _IOR(LKM_IOC_MAGIC, 2, int)

#endif
