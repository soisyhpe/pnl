#include "linux/sched.h"
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/pid.h>
#include <linux/printk.h>

MODULE_DESCRIPTION("Task monitor");
MODULE_AUTHOR("Eros Chan");
MODULE_LICENSE("GPL");

static pid_t target = 1;
module_param(target, int, 0660);
MODULE_PARM_DESC(target, "PID of the process to monitor");

struct task_monitor {
  struct pid *tm_pid; // descriptor for the process being monitored
};

struct task_monitor *t;
struct task_struct *ts;

// Will be executed by a kthread
int monitor_fn(void *arg) {
  int alive;

  // Get struct task_struct of the target process
  ts = get_pid_task(t->tm_pid, PIDTYPE_PID);

  if (ts == NULL) {
    pr_err("Error: No task_struct found for this process!\n");
    return -EINVAL;
  }

  // Get the process state
  alive = pid_alive(ts);

  // Check that the process still alive
  if (!alive) {
    pr_err("Error: Process %d is not alive!\n", ts->pid);
    return -EINVAL;
  }

  // Print the CPUi usage
  pr_info("pid %d usr %llu sys %llu\n", ts->pid, ts->utime, ts->stime);
  return 0;
}

// Gets the struct pid of target
int monitor_pid(pid_t pid) {
  t->tm_pid = find_get_pid(target);
  if (t->tm_pid == NULL)
    return -1;
  return 0;
}

// struct file_operations fops = {};

static int __init taskmonitor_init(void) { return 0; }

static void __exit taskmonitor_exit(void) {}

module_init(taskmonitor_init);
module_exit(taskmonitor_exit);
