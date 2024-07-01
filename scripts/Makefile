KERNELDIR_LKP ?= ~/pnl/vm/linux-6.5.7/
PWD := $(shell pwd)

module = hellosysfs
obj-m += ${module}.o

all:
	make -C $(KERNELDIR_LKP) M=$(PWD) modules
	cp ${module}.ko ~/pnl/vm/share/${module}.ko

clean:
	make -C $(KERNELDIR_LKP) M=$(PWD) clean
