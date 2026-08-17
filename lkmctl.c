#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "lkm_skeleton_ioctl.h"

#define DEVICE_PATH "/dev/lkm_skeleton"
#define READ_BUF_LEN 256

static void print_usage(const char *prog_name) {
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  %s write <text>   - write text to the device\n", prog_name);
    fprintf(stderr, "  %s read           - read current content from the device\n", prog_name);
    fprintf(stderr, "  %s size           - print current buffer size (via ioctl)\n", prog_name);
    fprintf(stderr, "  %s reset          - reset the device buffer (via ioctl)\n", prog_name);
}

// write()/read()/ioctl() below are syscalls - each one is the line that crosses from
// userspace into the kernel and gets dispatched to the matching lkm_* function.
static int cmd_write(int fd, const char *text) {
    ssize_t written = write(fd, text, strlen(text));
    if (written < 0) {
        perror("write");
        return 1;
    }
    printf("Wrote %zd bytes.\n", written);
    return 0;
}

static int cmd_read(int fd) {
    char buf[READ_BUF_LEN];
    ssize_t bytes_read = read(fd, buf, sizeof(buf) - 1);
    if (bytes_read < 0) {
        perror("read");
        return 1;
    }
    buf[bytes_read] = '\0';  // read() gives raw bytes, not a C string - terminate before printf
    printf("%s\n", buf);
    return 0;
}

static int cmd_size(int fd) {
    int size;
    if (ioctl(fd, LKM_IOC_GET_SIZE, &size) < 0) {
        perror("ioctl GET_SIZE");
        return 1;
    }
    printf("%d\n", size);
    return 0;
}

static int cmd_reset(int fd) {
    if (ioctl(fd, LKM_IOC_RESET) < 0) {
        perror("ioctl RESET");
        return 1;
    }
    printf("Buffer reset.\n");
    return 0;
}

int main(int argc, char *argv[]) {
    int fd;
    int ret;

    if (argc < 2) {  // argv[1] is the subcommand ("write"/"read"/"size"/"reset")
        print_usage(argv[0]);
        return 1;
    }

    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    if (strcmp(argv[1], "write") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: 'write' requires a text argument.\n");
            close(fd);
            return 1;
        }
        ret = cmd_write(fd, argv[2]);
    } else if (strcmp(argv[1], "read") == 0) {
        ret = cmd_read(fd);
    } else if (strcmp(argv[1], "size") == 0) {
        ret = cmd_size(fd);
    } else if (strcmp(argv[1], "reset") == 0) {
        ret = cmd_reset(fd);
    } else {
        fprintf(stderr, "Error: unknown command '%s'.\n", argv[1]);
        print_usage(argv[0]);
        ret = 1;
    }

    close(fd);
    return ret;
}
