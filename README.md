# char_driver

A Linux character device driver written in C, built and tested on WSL2/Ubuntu, together with a matching userspace command-line tool.

## Why I built this

I wanted to understand what actually happens beneath a `read()`/`write()`/`ioctl()` call - not
just how to use the Linux kernel API, but why it's shaped the way it is: why userspace pointers
can't be dereferenced directly, why a mutex isn't enough on its own without a wait queue for
blocking I/O, why `krealloc`'s return value has to be checked into a temporary pointer before
overwriting the original. This project was built incrementally, each mechanism added and
verified (via `dmesg`, not just "it compiled") before moving to the next, which is why the
driver grew from a bare module skeleton into one with dynamic memory management, synchronized
concurrent access, custom control commands, and blocking reads.

## Overview

This project implements `lkm_skeleton`, a loadable kernel module (LKM) that registers a character device at `/dev/lkm_skeleton`. It demonstrates the core mechanics of Linux character device drivers: dynamic device registration, the `file_operations` interface, safe kernel/userspace memory transfer, synchronization, custom `ioctl` commands, and dynamic kernel memory management.

A companion userspace CLI tool, `lkmctl`, wraps the driver's functionality in a convenient interface.

## Features

- Dynamic major/minor number allocation (`alloc_chrdev_region`)
- Automatic `/dev` node creation via `class_create` / `device_create` (no manual `mknod`)
- `open`, `release`, `read`, `write` implementations using `copy_to_user` / `copy_from_user`
- Thread-safe access via a kernel mutex, protecting shared driver state
- Custom `ioctl` commands (`RESET`, `GET_SIZE`) with a shared header between kernel and userspace
- Dynamically growing buffer using `kmalloc` / `krealloc` / `kfree`, with a bounded maximum size
- Zeroing of stale buffer contents on every write, to avoid leaking old data
- Blocking reads via a kernel wait queue: `read()` sleeps while the buffer is empty and wakes
  up once `write()` provides data, instead of returning EOF immediately
- Full and symmetric resource cleanup on module unload, including on every error path in `init`

## Project structure

```
lkm_skeleton.c          Kernel module: the character device driver itself
lkm_skeleton_ioctl.h     Shared ioctl command definitions (kernel + userspace)
lkmctl.c                 Userspace CLI tool (write / read / size / reset)
test_ioctl.c             Minimal userspace test program exercising the ioctl interface
Makefile                 Builds both the kernel module and the userspace tools
```

## Building

Requires kernel headers matching your running kernel (the `KDIR` variable in the `Makefile` points to them; defaults to `$(HOME)/wsl2-kernel`).

```bash
make
```

This builds `lkm_skeleton.ko` (the kernel module) and `lkmctl` (the CLI tool).

## Usage

```bash
sudo insmod lkm_skeleton.ko
sudo chmod 666 /dev/lkm_skeleton

./lkmctl write "hello kernel"
./lkmctl read
./lkmctl size
./lkmctl reset

sudo rmmod lkm_skeleton
```

Kernel log messages from the driver can be viewed with:

```bash
dmesg | tail
```

## Cleaning build artifacts

```bash
make clean
```
