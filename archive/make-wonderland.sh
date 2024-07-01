#! /bin/bash
DIR=/usr/data/sopena/pnl_2023-2024
WorkDir=/tmp/EROS 
if [ ! -d ${WorkDir} ];then
	mkdir -p ${WorkDir}
	chmod 700 ${WorkDir}
	cd ${WorkDir}
	cp ${DIR}/lkp-arch.img lkp-arch.img
	cp ${DIR}/linux-6.5.7.tar.xz linux-6.5.7.tar.xz
	tar xf linux-6.5.7.tar.xz
	rm linux-6.5.7/.config
  cp ~/pnl/.config linux-6.5.7/.config
	dd if=/dev/zero of=myHome.img bs=1M count=50
	/sbin/mkfs.ext4 myHome.img
  test -d ${WorkDir}/share || mkdir ${WorkDir}/share
	cd linux-6.5.7

	make -j 24
fi 

	

	
	 
