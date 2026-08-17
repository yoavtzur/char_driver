#include <stdio.h>      // printf, perror
#include <fcntl.h>      // open(), O_RDWR
#include <unistd.h>     // close()
#include <sys/ioctl.h>  // ioctl()
#include "lkm_skeleton_ioctl.h"

#define DEVICE_PATH "/dev/lkm_skeleton"

int main(void) {
    int fd;
    int size;

    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    if (ioctl(fd, LKM_IOC_GET_SIZE, &size) < 0) {
        perror("ioctl GET_SIZE");
        close(fd);
        return 1;
    }
    printf("Current buffer size: %d\n", size);

    if (ioctl(fd, LKM_IOC_RESET) < 0) {
        perror("ioctl RESET");
        close(fd);
        return 1;
    }
    printf("Buffer reset.\n");

    if (ioctl(fd, LKM_IOC_GET_SIZE, &size) < 0) {
        perror("ioctl GET_SIZE");
        close(fd);
        return 1;
    }
    printf("Buffer size after reset: %d\n", size);

    close(fd);
    return 0;
}
