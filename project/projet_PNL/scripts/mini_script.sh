# Mount point directory
mount_point="disk"

[[ -d $mount_point ]] || mkdir $mount_point

umount disk
rmmod ouichefs

insmod ouichefs.ko
mount disk.img disk

#
# ioctl
#

# fileinfo ioctl
mknod /dev/fileinfo c 248 0
gcc -Wall fileinfo_user.c -o fileinfo_user

# defrag ioctl
mknod /dev/defrag c 247 0
gcc -Wall defrag_user.c -o defrag_user

# Benchmark
make clean
make && ./benchmark

# ./fileinfo_user disk/file3
# ./defrag_user disk/file3
# ./fileinfo_user disk/file3

# Cas basique
# echo "Melissa" > disk/melissa
# echo "Eros" > disk/eros
# echo "Nicolas" > disk/nicolas

# cat disk/melissa
# cat disk/eros
# cat disk/nicolas

# Cas simple
# echo "Check that your implementation works correctly using the test bench you wrote in the previous step and measure the performance loss compared to the default implementation." > disk/bonjour
# sync # workaround pour régler le problème du cache (à utiliser sans le write)
# cat disk/bonjour #> bonjour.result


