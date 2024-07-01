KERNELDIR_LKP ?= /tmp/EROS/linux-6.5.7/
PWD := $(shell pwd)

module = taskmonitor
obj-m += ${module}.o

all:
	make -C $(KERNELDIR_LKP) M=$(PWD) modules
	cp ${module}.ko /tmp/EROS/share/${module}.ko

clean:
	make -C $(KERNELDIR_LKP) M=$(PWD) clean
