#include <linux/init.h>
// #include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/pid.h>
// #include <linux/sched.h>
#include <linux/delay.h>

#include "taskmonitor.h"

MODULE_DESCRIPTION("Task monitor");
MODULE_AUTHOR("Eros Chan");
MODULE_LICENSE("GPL");

static pid_t target = 1;
module_param(target, int, 0000);

struct task_monitor tm;
// struct task_struct *ts;

int monitor_fn(void *arg) {
  struct task_struct *ts;
  int is_alive;

  ts = get_pid_task(
      tm.tm_pid,
      PIDTYPE_PID); // This function increments the reference counter of the
                    // task_struct. We need to put this reference using
                    // put_task_struct() function.

  if (ts == NULL) {
    return -EINVAL;
  }

  is_alive = pid_alive(ts);

  if (is_alive) {
    // pr_err("Error : Process with pid %d isn't alive!\n", tm.tm_pid);
    return -EINVAL;
  }

  while (!kthread_should_stop()) {
    // The process is alive
    // pr_info("pid %d usr %llu sys %llu\n", tm.tm_pid, ts->utime, ts->stime);
    pr_info("test\n");
    ssleep(10);
  }

  return 0;
}

// Gets the struct pid of target
int monitor_pid(pid_t pid) {
  tm.tm_pid = find_get_pid(
      pid); // This function increments the reference counter of the struct pid.
            // We need to put this reference using put_pid() function.

  // No process has this pid value
  if (tm.tm_pid == NULL)
    return -1;

  return 0;
}

// struct file_operations fops = {};

static int __init taskmonitor_init(void) {
  // pr_info("test=%d\n",target);
  kthread_run(&monitor_fn, NULL, "monitor_fn");
  return 0;
}
module_init(taskmonitor_init);

static void __exit taskmonitor_exit(void) {}
module_exit(taskmonitor_exit);
