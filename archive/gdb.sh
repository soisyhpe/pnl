#! /bin/bash

gdb -ex 'target remote :1234' /tmp/EROS/linux-6.5.7/vmlinux
