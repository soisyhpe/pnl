#! /bin/bash

if [ -z "$1" ]; then
    echo "Usage: $0 <vm-name>"
    exit -1
fi

if [ -z "$2" ]; then
    echo "Usage: $0 $1 <port>"
    exit -1
fi

WORKING_DIR=$PNL_VMS_DIR/$1

# Fix the paths if necessary
HDA="-drive file=${WORKING_DIR}/lkp-arch.img,format=raw"
HDB="-drive file=${WORKING_DIR}/myHome.img,format=raw"
SHARED="${WORKING_DIR}/share"
KERNEL=${WORKING_DIR}/linux-6.5.7/arch/x86/boot/bzImage

# KDB=1
if [ -z ${KDB} ]; then
    CMDLINE='root=/dev/sda1 rw console=ttyS0 kgdboc=ttyS1'
 else
    CMDLINE='root=/dev/sda1 rw console=ttyS0 kgdboc=ttyS1 kgdbwait'
fi

# Used to accelerate ouichefs development
# CMDLINE+=" cd /share"

FLAGS="--enable-kvm -nographic"
VIRTFS+=" --virtfs local,path=${SHARED},mount_tag=share,security_model=passthrough,id=share "

# Replace mon:stdio to none to remove logs
exec qemu-system-x86_64 ${FLAGS} \
    ${HDA} ${HDB} \
    ${VIRTFS} \
    -net user -net nic \
    -serial mon:stdio \
    -boot c -m 1G \
    -kernel "${KERNEL}" \
    -append "${CMDLINE}" \
