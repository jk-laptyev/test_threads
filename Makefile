ifneq ($(KERNELRELEASE),)
# kbuild part of makefile
obj-m  := test_threads.o test_threads_locked.o
else
# normal makefile
KDIR ?= /home/ilaptiev/training/kernel/linux-stable

default:
	$(MAKE) -C $(KDIR) M=$$PWD
clean:
	$(MAKE) -C $(KDIR) M=$$PWD clean
endif