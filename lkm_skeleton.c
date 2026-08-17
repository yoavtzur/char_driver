#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include "lkm_skeleton_ioctl.h"

#define DEVICE_NAME "lkm_skeleton"
#define INITIAL_BUF_LEN 256
#define MAX_BUFFER_SIZE (64 * 1024)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yoav Tzur");
MODULE_DESCRIPTION("Milestone 2: Character Device Registration");
MODULE_VERSION("2.0");

static dev_t dev_num;
static struct cdev my_cdev;
static struct class *my_class;

static char *device_buffer;
static size_t buffer_capacity = 0;
static size_t buffer_data_size = 0;
static DEFINE_MUTEX(lkm_mutex);

static int lkm_open(struct inode *inode, struct file *file) {
    printk(KERN_INFO "LKM Skeleton: Device opened.\n");
    return 0;
}

static int lkm_release(struct inode *inode, struct file *file) {
    printk(KERN_INFO "LKM Skeleton: Device closed.\n");
    return 0;
}

static ssize_t lkm_read(struct file *file, char __user *user_buf, size_t count, loff_t *offset) {
    ssize_t bytes_to_copy;

    if (mutex_lock_interruptible(&lkm_mutex))
        return -ERESTARTSYS;

    if (*offset >= buffer_data_size) {
        mutex_unlock(&lkm_mutex);
        return 0;
    }

    bytes_to_copy = min((size_t)(buffer_data_size - *offset), count);

    if (copy_to_user(user_buf, device_buffer + *offset, bytes_to_copy) != 0) {
        mutex_unlock(&lkm_mutex);
        return -EFAULT;
    }

    *offset += bytes_to_copy;
    printk(KERN_INFO "LKM Skeleton: Sent %zd bytes to user.\n", bytes_to_copy);

    mutex_unlock(&lkm_mutex);
    return bytes_to_copy;
}

static ssize_t lkm_write(struct file *file, const char __user *user_buf, size_t count, loff_t *offset) {
    size_t bytes_to_copy = min((size_t)count, (size_t)(MAX_BUFFER_SIZE - 1));
    size_t needed_capacity = bytes_to_copy + 1;

    if (mutex_lock_interruptible(&lkm_mutex))
        return -ERESTARTSYS;

    if (needed_capacity > buffer_capacity) {
        char *new_buf = krealloc(device_buffer, needed_capacity, GFP_KERNEL);
        if (!new_buf) {
            mutex_unlock(&lkm_mutex);
            return -ENOMEM;
        }
        device_buffer = new_buf;
        buffer_capacity = needed_capacity;
        printk(KERN_INFO "LKM Skeleton: Buffer grown to %zu bytes.\n", buffer_capacity);
    }

    if (copy_from_user(device_buffer, user_buf, bytes_to_copy) != 0) {
        mutex_unlock(&lkm_mutex);
        return -EFAULT;
    }

    buffer_data_size = bytes_to_copy;
    device_buffer[buffer_data_size] = '\0';
    memset(device_buffer + buffer_data_size + 1, 0, buffer_capacity - buffer_data_size - 1);

    printk(KERN_INFO "LKM Skeleton: Received %zu bytes from user.\n", bytes_to_copy);

    mutex_unlock(&lkm_mutex);
    return bytes_to_copy;
}

static long lkm_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    int current_size;

    switch (cmd) {
    case LKM_IOC_RESET:
        if (mutex_lock_interruptible(&lkm_mutex))
            return -ERESTARTSYS;
        buffer_data_size = 0;
        memset(device_buffer, 0, buffer_capacity);
        mutex_unlock(&lkm_mutex);
        printk(KERN_INFO "LKM Skeleton: Buffer reset via ioctl.\n");
        break;

    case LKM_IOC_GET_SIZE:
        if (mutex_lock_interruptible(&lkm_mutex))
            return -ERESTARTSYS;
        current_size = buffer_data_size;
        mutex_unlock(&lkm_mutex);
        if (copy_to_user((int __user *)arg, &current_size, sizeof(current_size)))
            return -EFAULT;
        printk(KERN_INFO "LKM Skeleton: Reported size %d via ioctl.\n", current_size);
        break;

    default:
        return -ENOTTY;
    }

    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = lkm_open,
    .release = lkm_release,
    .read = lkm_read,
    .write = lkm_write,
    .unlocked_ioctl = lkm_ioctl,
};

static int __init lkm_skeleton_init(void) {
    int ret;

    device_buffer = kmalloc(INITIAL_BUF_LEN, GFP_KERNEL);
    if (!device_buffer) {
        printk(KERN_ALERT "LKM Skeleton: Failed to allocate initial buffer.\n");
        return -ENOMEM;
    }
    buffer_capacity = INITIAL_BUF_LEN;
    memset(device_buffer, 0, buffer_capacity);

    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        printk(KERN_ALERT "LKM Skeleton: Failed to allocate a major number.\n");
        kfree(device_buffer);
        return ret;
    }
    printk(KERN_INFO "LKM Skeleton: Allocated major number %d, minor number %d.\n",
           MAJOR(dev_num), MINOR(dev_num));

    cdev_init(&my_cdev, &fops);
    my_cdev.owner = THIS_MODULE;

    ret = cdev_add(&my_cdev, dev_num, 1);
    if (ret < 0) {
        printk(KERN_ALERT "LKM Skeleton: Failed to add the cdev to the kernel.\n");
        unregister_chrdev_region(dev_num, 1);
        kfree(device_buffer);
        return ret;
    }

    my_class = class_create(DEVICE_NAME);
    if (IS_ERR(my_class)) {
        printk(KERN_ALERT "LKM Skeleton: Failed to create device class.\n");
        cdev_del(&my_cdev);
        unregister_chrdev_region(dev_num, 1);
        kfree(device_buffer);
        return PTR_ERR(my_class);
    }

    if (IS_ERR(device_create(my_class, NULL, dev_num, NULL, DEVICE_NAME))) {
        printk(KERN_ALERT "LKM Skeleton: Failed to create the device.\n");
        class_destroy(my_class);
        cdev_del(&my_cdev);
        unregister_chrdev_region(dev_num, 1);
        kfree(device_buffer);
        return -1;
    }

    printk(KERN_INFO "LKM Skeleton: Module loaded into kernel space.\n");
    return 0;
}

static void __exit lkm_skeleton_exit(void) {
    device_destroy(my_class, dev_num);
    class_destroy(my_class);
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev_num, 1);
    kfree(device_buffer);
    printk(KERN_INFO "LKM Skeleton: Module unloaded from kernel space.\n");
}

module_init(lkm_skeleton_init);
module_exit(lkm_skeleton_exit);
