savedcmd_/root/pnl/labs/7/weasel_procfs.ko := ld -r -m elf_x86_64 -z noexecstack --no-warn-rwx-segments --build-id=sha1  -T scripts/module.lds -o /root/pnl/labs/7/weasel_procfs.ko /root/pnl/labs/7/weasel_procfs.o /root/pnl/labs/7/weasel_procfs.mod.o;  make -f ./arch/x86/Makefile.postlink /root/pnl/labs/7/weasel_procfs.ko