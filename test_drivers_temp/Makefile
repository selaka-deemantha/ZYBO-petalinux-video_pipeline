obj-m := dyn_clk_test.o
KDIR := /home/selaka/petalinux/2021.2/fb_reset_gpio/build/tmp/work-shared/zynq-generic/kernel-source
PWD := $(shell pwd)
EXTRA_CFLAGS += -fno-stack-protector
all:
	make -C $(KDIR) M=$(PWD) ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- modules

clean:
	make -C $(KDIR) M=$(PWD) clean
