savedcmd_/root/pnl/labs/5/task-4/taskmonitor.mod := printf '%s\n'   taskmonitor.o | awk '!x[$$0]++ { print("/root/pnl/labs/5/task-4/"$$0) }' > /root/pnl/labs/5/task-4/taskmonitor.mod
