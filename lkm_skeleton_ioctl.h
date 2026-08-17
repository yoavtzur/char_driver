#ifndef LKM_SKELETON_IOCTL_H
#define LKM_SKELETON_IOCTL_H

#include <linux/ioctl.h>

#define LKM_IOC_MAGIC 'k'

#define LKM_IOC_RESET    _IO(LKM_IOC_MAGIC, 1)
#define LKM_IOC_GET_SIZE _IOR(LKM_IOC_MAGIC, 2, int)

#endif
