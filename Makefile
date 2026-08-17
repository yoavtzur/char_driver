obj-m += lkm_skeleton.o

KDIR ?= $(HOME)/wsl2-kernel
PWD := $(shell pwd)

all: module lkmctl

module:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

lkmctl: lkmctl.c lkm_skeleton_ioctl.h
	gcc -Wall -o lkmctl lkmctl.c

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f lkmctl
