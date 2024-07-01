#! /bin/bash

# Fix the paths if necessary
HDA="-drive file=/tmp/EROS/lkp-arch.img,format=raw"
HDB="-drive file=/tmp/EROS/myHome.img,format=raw"
SHARED="/tmp/EROS/share"
KERNEL=/tmp/EROS/linux-6.5.7/arch/x86/boot/bzImage

# KDB=1
# if [ -z ${KDB} ]; then
    CMDLINE='root=/dev/sda1 rw console=ttyS0 kgdboc=ttyS1'
# else
    # CMDLINE='root=/dev/sda1 rw console=ttyS0 kgdboc=ttyS1 kgdbwait'
# fi

# CMDLINE+=' kgdbwait'
FLAGS="--enable-kvm -nographic"
VIRTFS+=" --virtfs local,path=${SHARED},mount_tag=share,security_model=passthrough,id=share "

exec qemu-system-x86_64 ${FLAGS} \
     ${HDA} ${HDB} \
     ${VIRTFS} \
     -net user -net nic \
     -serial mon:stdio -serial tcp::1234,server,nowait \
     -boot c -m 1G \
     -kernel "${KERNEL}" \
     -append "${CMDLINE}"
