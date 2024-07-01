#ifndef TASKMONITOR_HEADER
#define TASKMONITOR_HEADER

struct task_monitor {
	struct pid *tm_pid; // Serve as a descriptor for the process being monitored
};

#endif
