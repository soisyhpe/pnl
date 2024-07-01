WORKING_DIR = ~/pnl/vms/eros
KERNELDIR_LKP ?= linux-6.5.7
PWD := $(shell pwd)

module = weasel_procfs
obj-m += ${module}.o

all:
	make -j 16 -C $(WORKING_DIR)/$(KERNELDIR_LKP) M=$(PWD) modules
	cp ${module}.ko ${WORKING_DIR}/share/${module}.ko

clean:
	make -j 16 -C $(WORKING_DIR)/$(KERNELDIR_LKP) M=$(PWD) clean
