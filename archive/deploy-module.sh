KERNEL_DIR ?= /tmp/EROS/linux-6.5.7

all:
    make -C $(KERNEL_DIR) M=$PWD
