#! /bin/bash

# sudo apt-get install qemu-system
# sudo apt-get install clang
# sudo apt-get install make
# sudo apt-get install gdb
# sudo apt-get install gcc
# sudo apt-get install libncurses5-dev
# sudo apt-get install bison
# sudo apt-get install flex
# sudo apt-get install libssl-dev
# sudo apt-get install libelf-dev
# sudo apt-get update
# sudo apt-get upgrade

if [ -z "$1" ]; then
	echo "Usage: $0 <vm-name>"
	exit -1
fi

WORKING_DIR=$PNL_DIR/$1

N_CPU=$(nproc)

if [ ! -d ${WORKING_DIR} ];then
	mkdir -p ${WORKING_DIR}
	
	chmod 700 ${WORKING_DIR}
	
	cd ${WORKING_DIR}
	cp ${RESOURCES_DIR}/lkp-arch.img lkp-arch.img
	cp ${RESOURCES_DIR}/linux-6.5.7.tar.xz linux-6.5.7.tar.xz
	
	tar xf linux-6.5.7.tar.xz
	
	# rm linux-6.5.7/.config
	
  cp ${RESOURCES_DIR}/.config linux-6.5.7/.config
	
	dd if=/dev/zero of=myHome.img bs=1M count=50
	
	/sbin/mkfs.ext4 myHome.img
	
  test -d ${WORKING_DIR}/share || mkdir ${WORKING_DIR}/share
	
	cd linux-6.5.7

	# make CC=clang -j $((2*N_CPU))
	make -j $((2*N_CPU))
else
	cd ${WORKING_DIR}/linux-6.5.7
	# make CC=clang -j $((2*N_CPU))
	make -j $((2*N_CPU))
fi 
