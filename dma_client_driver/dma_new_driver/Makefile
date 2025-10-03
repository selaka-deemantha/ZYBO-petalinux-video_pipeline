obj-m := fft_dma2.o
KDIR := /home/selaka/petalinux/2024/zybo_new/build/tmp/work-shared/zynq-generic-7z010/kernel-source


PWD := $(shell pwd)


MODULE := fft_dma2.ko

EXTRA_CFLAGS += -fno-stack-protector




all:
	make -C $(KDIR) M=$(PWD) ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- modules

clean:
	make -C $(KDIR) M=$(PWD) clean

