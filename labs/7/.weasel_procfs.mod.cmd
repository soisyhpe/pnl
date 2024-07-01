savedcmd_/root/pnl/labs/7/weasel_procfs.mod := printf '%s\n'   weasel_procfs.o | awk '!x[$$0]++ { print("/root/pnl/labs/7/"$$0) }' > /root/pnl/labs/7/weasel_procfs.mod
