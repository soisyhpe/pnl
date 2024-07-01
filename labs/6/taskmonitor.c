#ifndef TASKMONITOR_H
#define TASKMONITOR_H
#include <linux/types.h>

struct task_monitor {
	struct pid* p;
};

struct task_sample {
	u64 utime;
	u64 stime;
	struct list_head list;
	int size;
};

#endif
