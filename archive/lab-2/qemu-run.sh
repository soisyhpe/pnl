#! /bin/bash

# Fix the paths if necessary
HDA="-drive file=/tmp/lkp-arch.img,format=raw"
HDB="-drive file=/tmp/myHome.img,format=raw"
FLAGS="--enable-kvm "

exec qemu-system-x86_64 ${FLAGS} \
     ${HDA} ${HDB} \
     -net user -net nic \
     -boot c -m 1G
