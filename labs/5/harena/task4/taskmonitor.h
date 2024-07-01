#ifndef TASKMONITOR_H_
#define TASKMONITOR_H_
#include <linux/types.h>

struct task_monitor {
	struct pid* p;
};

struct task_sample {
	u64 utime;
	u64 stime;
};

#endif
