obj-m += ouichefs.o
ouichefs-objs := fs.o super.o inode.o file.o dir.o block.o fileinfo.o defrag.o

VM ?= $(shell bash -c 'read -p "VM: " vm; echo $$vm')
WORKING_DIR = /root/pnl/vms
KERNEL_DIR = linux-6.5.7
# KERNELDIR ?= /lib/modules/$(shell uname -r)/build
# KERNELDIR ?= ~/pnl/vms/eros-project/lib/modules/$(shell uname -r)/build

all:
	make -C $(WORKING_DIR)/$(VM)/$(KERNEL_DIR) M=$(PWD) modules

debug:
	make -C $(WORKING_DIR)/$(VM)/$(KERNEL_DIR) M=$(PWD) ccflags-y+="-DDEBUG -g" modules

clean:
	make -C $(WORKING_DIR)/$(VM)/$(KERNEL_DIR) M=$(PWD) clean
	rm -rf *~

.PHONY: all clean
