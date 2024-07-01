#include "taskmonitor.h"
#include "asm-generic/errno-base.h"
#include "linux/cleanup.h"
#include "linux/delay.h"
#include "linux/kobject.h"
#include "linux/kthread.h"
#include "linux/pid.h"
#include "linux/rculist.h"
#include "linux/sched.h"
#include "linux/sched/task.h"
#include "linux/slab.h"
#include "linux/string.h"
#include "linux/sysctl.h"
#include "linux/sysfs.h"
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h> // Module metadata
#include <linux/uaccess.h>
MODULE_AUTHOR("Harena RAKOTONDRATSIMA");
MODULE_DESCRIPTION("Hello world module");
MODULE_LICENSE("GPL");

static pid_t target = 1;
module_param(target, int, 0660);
MODULE_PARM_DESC(target, "PID of the process to monitor");

struct task_monitor t;
static struct task_struct *ktmonitor_thread;
struct task_struct *ts;
struct task_sample t_sample;
char err;
char cmd[100];
char get_sample(struct task_struct *tm, struct task_sample *ts)
{
	if (pid_alive(tm)) {
		ts->stime = tm->stime;
		ts->utime = tm->utime;
		return 1;
	}
	return 0;
}

static ssize_t show_task_monitor(struct kobject *kobj,
				 struct kobj_attribute *attr, char *buf)
{
	if (get_sample(ts, &t_sample))
		return snprintf(buf, PAGE_SIZE,
				"pid : %d  user : %llu  sys : %llu\n", ts->pid,
				t_sample.utime, t_sample.stime);
	return snprintf(buf, PAGE_SIZE,
			"the process : %d is not alive anymore\n", target);
}

int monitor_fn(void *arg)
{
	if (ts == NULL) {
		pr_err("monitor_fn : No task_struct for this process");
		return EINVAL;
	}

	while (get_sample(ts, &t_sample) && (!kthread_should_stop())) {
		pr_info("pid : %d  user : %llu  sys : %llu\n", ts->pid,
			t_sample.utime, t_sample.stime);
		ssleep(10);
	}
	return 0;
}

static ssize_t store_cmd(struct kobject *kobj, struct kobj_attribute *attr,
			 const char *buf, size_t count)
{
	ssize_t cpy;
	memset(cmd, 0, 100);
	cpy = snprintf(cmd, count + 1, buf);
	if (strcmp("start", cmd) == 0) {
		if (ktmonitor_thread) {
			pr_info("The thread monitor is already running\n");
			return cpy;
		}
		pr_info("Starting the monitoriing thread");
		ktmonitor_thread = kthread_run(monitor_fn, NULL, "monitor_fn");
		return cpy;
	}

	if (strcmp("stop", cmd) == 0) {
		if (ktmonitor_thread) {
			kthread_stop(ktmonitor_thread);
			/*put_task_struct(ktmonitor_thread);*/
			ktmonitor_thread = NULL;
			pr_info("Thread monitor stopped\n");
			return cpy;
		}
		pr_info("The thread monitor is already stopped\n");
	} else {
		pr_err("Command not found\n");
	}
	return cpy;
}
static struct kobj_attribute system_t_monitor_attr =
	__ATTR(taskmonitor, 0600, show_task_monitor, store_cmd);

int monitor_pid(pid_t pid)
{
	t.p = find_get_pid(pid);
	if (t.p == NULL)
		return -1;

	return 0;
}

static int __init tm_init(void)
{
	int ret_val0 =
		sysfs_create_file(kernel_kobj, &system_t_monitor_attr.attr);
	if (ret_val0) {
		goto err3;
		err = 1;
		return EINVAL;
	}
	int ret_val1 = monitor_pid(target);
	if (ret_val1 == -1) {
		err = 1;
		pr_err("monitor_pid : pid does not exist\n");
		goto err2;
	}

	ts = get_pid_task(t.p, PIDTYPE_PID);
	if (ts == NULL) {
		err = 1;
		pr_err("tm_init : No task_struct for this process");
		goto err1;
	}

	ktmonitor_thread = kthread_run(monitor_fn, NULL, "monitor_fn");
	return 0;
err1:
	put_pid(t.p);
err2:
	sysfs_remove_file(kernel_kobj, &system_t_monitor_attr.attr);
err3:
	return EINVAL;
}

static void __exit tm_exit(void)
{
	if (err) {
		pr_info("Module buggy unloaded\n");
		return;
	}

	if (ktmonitor_thread) {
		kthread_stop(ktmonitor_thread);
	}

	put_pid(t.p);
	sysfs_remove_file(kernel_kobj, &system_t_monitor_attr.attr);
	put_task_struct(ts);
	pr_warn("Task Monitor module unloaded\n");
	return;
}
module_init(tm_init);
module_exit(tm_exit);
