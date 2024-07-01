savedcmd_/root/pnl/labs/7/task-3/rootkit.mod := printf '%s\n'   rootkit.o | awk '!x[$$0]++ { print("/root/pnl/labs/7/task-3/"$$0) }' > /root/pnl/labs/7/task-3/rootkit.mod
