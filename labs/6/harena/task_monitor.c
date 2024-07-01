#include "asm-generic/errno-base.h"
#include "asm/ptrace.h"
#include "asm/unwind.h"
#include "linux/cleanup.h"
#include "linux/container_of.h"
#include "linux/debugfs.h"
#include "linux/delay.h"
#include "linux/gfp_types.h"
#include "linux/jump_label.h"
#include "linux/kobject.h"
#include "linux/kref.h"
#include "linux/kstrtox.h"
#include "linux/kthread.h"
#include "linux/list.h"
#include "linux/lockdep.h"
#include "linux/mempool.h"
#include "linux/mutex.h"
#include "linux/pid.h"
#include "linux/sched.h"
#include "linux/sched/task.h"
#include "linux/seq_file.h"
#include "linux/sfp.h"
#include "linux/shrinker.h"
#include "linux/slab.h"
#include "linux/string.h"
#include "linux/sysfs.h"
#include "linux/types.h"
#include "taskmonitor.h"
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h> // Module metadata
#include <linux/uaccess.h>
#define SIZE 40
MODULE_AUTHOR("Harena RAKOTONDRATSIMA");
MODULE_DESCRIPTION("TaskMonitor  module");
MODULE_LICENSE("GPL");

static pid_t target = 1;
module_param(target, int, 0660);
MODULE_PARM_DESC(target, "PID of the process to monitor");

static struct task_struct *ktmonitor_thread;
char cmd[100];
unsigned long num;
static struct kmem_cache *taskmonitor_cache;
static struct kmem_cache *taskmonitor_cache_tm;
static mempool_t *taskmonitor_mempool;
static mempool_t *taskmonitor_mempool_tm;
struct dentry *taskmonitor_debugfs;
struct mutex *mtx_taskmonitor;
static struct list_head *lh_task_monitor = NULL;
static struct list_head *head_sample = NULL;
LIST_HEAD(head_taskmonitor);
char monitor_pid(pid_t pid, struct task_monitor *t) {
  t->p = find_get_pid(pid);
  if (t->p == NULL)
    return 1;

  return 0;
}

/*release function for kref of task_sample*/
void release(struct kref *kref) {
  struct task_sample *del = container_of(kref, struct task_sample, kref);
  list_del(&del->list);
  mempool_free(del, taskmonitor_mempool);
}
void release_taskmonitor(struct kref *kref) {
  struct task_monitor *del = container_of(kref, struct task_monitor, kref_tm);

  struct task_sample *it, *tmp = NULL;
  list_for_each_entry_safe(it, tmp, &del->head, list) {
    kref_put(&it->kref, release);
  }
  put_task_struct(del->ts_monitored);
  put_pid(del->p);
  list_del(&del->tasks);
  mempool_free(del, taskmonitor_mempool_tm);
}

/*Seq_file definition*/
static void *taskmonitor_seq_start(struct seq_file *seq, loff_t *pos) {
  mutex_lock(mtx_taskmonitor);
  /*Doesn't allocate dynamic memory*/
  struct list_head *lh;
  struct task_monitor *tm;
  loff_t pos_bis = *pos;
  list_for_each(lh, &head_taskmonitor) {
    if (pos_bis-- == 0) {
      tm = container_of(lh, struct task_monitor, tasks);
      head_sample = &tm->head;
      // mutex_lock(&tm->mx);
      lh_task_monitor = lh;
      return head_sample->next;
    }
  }
  return NULL;
}

static void *taskmonitor_seq_next(struct seq_file *seq, void *v, loff_t *pos) {
  struct list_head *prev = (struct list_head *)v;
  struct list_head *res = prev;
  struct task_monitor *tm;
  res = res->next;
  ++*pos;
  if (res == head_sample) {
    lh_task_monitor = lh_task_monitor->next;
    if (lh_task_monitor == &head_taskmonitor) {
      return NULL;
    }
    tm = container_of(lh_task_monitor, struct task_monitor, tasks);
    head_sample = &tm->head;
    res = head_sample->next;
  }
  return res;
}

static void taskmonitor_seq_stop(struct seq_file *seq, void *v) {
  mutex_unlock(mtx_taskmonitor);
}

static int taskmonitor_seq_show(struct seq_file *seq, void *v) {
  struct task_sample *ts = list_entry(v, struct task_sample, list);
  kref_get(&ts->kref);
  seq_printf(seq,
             "%ld - pid : %d usr %llu sys %llu\n\ttotal mem : %lu, data : %lu "
             ", stack : %lu\n",
             ts->num, ts->pid, ts->utime, ts->stime, ts->mm->total_vm,
             ts->mm->data_vm, ts->mm->stack_vm);
  kref_put(&ts->kref, release);
  return 0;
}

static const struct seq_operations taskmonitor_seq_ops = {

    .start = taskmonitor_seq_start,
    .next = taskmonitor_seq_next,
    .stop = taskmonitor_seq_stop,
    .show = taskmonitor_seq_show,
};
/*End of seq_file definition*/

/*Function to be called when opening the debugfs file*/
static int taskmonitor_debugfs_open(struct inode *inode, struct file *file) {
  return seq_open(file, &taskmonitor_seq_ops);
}

/*Write function for the debugfs*/
static ssize_t taskmonitor_debugfs_write(struct file *file,
                                         const char __user *user_buf,
                                         size_t size, loff_t *ppos) {
  char buf[32];
  int buf_size;
  /*find = 1 if we do found the process*/
  char find = 0;
  /*
   *cmd = 0 : add process to the monitoring list
   *cmd = 1 : take out the process to the monitoring list
   */
  char cmd = 0;
  struct list_head *it;
  struct task_monitor *tm;
  pid_t pid;
  struct task_monitor *new;
  buf_size = min(size, (sizeof(buf) - 1));
  if (strncpy(buf, user_buf, buf_size) < 0)
    return -EFAULT;

  buf[buf_size] = 0;

  if (buf[0] == '-') {
    if (kstrtouint(buf + 1, 0, &pid)) {
      pr_err("tasmonitor_cmd_debugfs : process number bad format\n");
      return -EINVAL;
    }
    cmd = 1;
  }

  else {
    if (kstrtouint(buf, 0, &pid)) {
      pr_err("tasmonitor_cmd_debugfs : process number bad format\n");
      return -EINVAL;
    }
  }
  mutex_lock(mtx_taskmonitor);
  list_for_each(it, &head_taskmonitor) {
    tm = container_of(it, struct task_monitor, tasks);
    /*Looking for our process*/
    if (tm->ts_monitored->pid == pid) {
      find = 1;
      break;
    }
  }
  mutex_unlock(mtx_taskmonitor);

  switch (cmd) {
  /*Remove process*/
  case 1:
    switch (find) {
    case 0:
      pr_err("taskmonitor_cmd_debugfs : process not found\n");
      return -EINVAL;
    case 1:
      /*remove taskmonitor*/
      kref_put(&tm->kref_tm, release_taskmonitor);
      pr_info("process %d will be removed from the monitoring list\n", pid);
      break;
    }
    break;
  /*Add process*/
  case 0:
    switch (find) {
    case 0:
      new = mempool_alloc(taskmonitor_mempool_tm, GFP_KERNEL);
      if (monitor_pid(pid, new) == 1) {
        pr_err("monitor_pid : pid does not exist\n");
        goto err_monitor_pid;
      }

      new->ts_monitored = get_pid_task(new->p, PIDTYPE_PID);
      if (new->ts_monitored == NULL) {
        pr_err("tm_init : No task_struct for this process");
        goto err_get_pid_task;
      }
      pr_info("process %d will be added to the monitoring list\n", pid);
      mutex_init(&new->mx);
      INIT_LIST_HEAD(&new->head);
      kref_init(&new->kref_tm);
      /*Usefull for the shrinker*/
      new->count = 0;
      new->size = 0;
      new->num = 0;
      new->to_shrink = 0;
      mutex_lock(mtx_taskmonitor);
      list_add_tail(&new->tasks, &head_taskmonitor);
      mutex_unlock(mtx_taskmonitor);
      break;

    case 1:
      pr_info("process %d is already into the monitoring list\n", pid);
    }
  }

  return buf_size;

err_get_pid_task:
  put_pid(new->p);

err_monitor_pid:
  /*remove the taskmonitor*/
  kref_put(&new->kref_tm, release_taskmonitor);
  return -EINVAL;
}

static const struct file_operations taskmonitor_fops = {
    .owner = THIS_MODULE,
    .open = taskmonitor_debugfs_open,
    .read = seq_read,
    .write = taskmonitor_debugfs_write,
    .llseek = seq_lseek,
    .release = seq_release,
};

int count(struct task_monitor *tm) {
  int count = tm->count;
  // ssize_t size = tm->size;
  ssize_t size = SIZE;
  if (count * size < PAGE_SIZE)
    return 0;

  /*Remove elements in our task_sample if it exceed on PAGE_SIZE to print them*/
  return (unsigned long)(((count * size) - PAGE_SIZE) / size);
}

/*
 *Declaration of the taskmonitor shrinker
 *For each taskmonitor, couunt the number of list to shrink and into
 *shrink_scan, shrink those list*/
static unsigned long taskmonitor_shrink_count(struct shrinker *s,
                                              struct shrink_control *sc) {
  int res = 0;
  int tmp = 0;
  struct task_monitor *tm;
  list_for_each_entry(tm, &head_taskmonitor, tasks) {
    tmp = count(tm);
    tm->to_shrink = tmp;
    res += tmp;
  }

  if (res == 0)
    return SHRINK_EMPTY;

  return res;
}

unsigned long shrink_scan_task_monitor(struct task_monitor *tm) {
  unsigned long cpt = 0;
  struct task_sample *it, *tmp = NULL;
  list_for_each_entry_safe(it, tmp, &tm->head, list) {
    /*Put the reference, not deleting it immediatly in case someone
     *else is using the struct */
    kref_put(&it->kref, release);
    if (cpt >= tm->to_shrink) {
      tm->count -= tm->to_shrink;
      tm->to_shrink = 0;
      break;
    }
    cpt++;
  }
  return cpt;
}
/*
 *nr_to_scan : total of list shrinkable
 *shrink only the right amount for each task_monitor
 */

static unsigned long taskmonitor_shrink_scan(struct shrinker *s,
                                             struct shrink_control *sc) {
  unsigned long cpt = 0;

  struct task_monitor *it = NULL;
  /*Check if there is no deadlock with the list mutex of taskmonitor*/
  if (!mutex_trylock(mtx_taskmonitor))
    return SHRINK_STOP;
  list_for_each_entry(it, &head_taskmonitor, tasks) {
    cpt += shrink_scan_task_monitor(it);
  }
  sc->nr_scanned -= cpt;
  if (sc->nr_scanned == 0)
    pr_info("shrink_scan : ok\n");
  mutex_unlock(mtx_taskmonitor);
  return cpt;
}

static struct shrinker taskmonitor_shrinker = {
    .count_objects = taskmonitor_shrink_count,
    .scan_objects = taskmonitor_shrink_scan,
    .seeks = DEFAULT_SEEKS,
};

/*End of declaration of taskmonitor shrinker*/

static char get_sample(struct task_struct *tm, struct task_sample *ts) {
  if (pid_alive(tm)) {
    ts->stime = tm->stime;
    ts->utime = tm->utime;
    ts->mm = tm->mm;
    return 1;
  }
  return 0;
}

static int save_sample(struct task_monitor *tm) {
  /**
   *Allocating with mempool_alloc but only the system know
   *if he shoulds take memory from the mempool or
   *use the usual allocating
   */
  struct task_sample *new = mempool_alloc(taskmonitor_mempool, GFP_KERNEL);
  char ret_val = get_sample(tm->ts_monitored, new);
  new->num = tm->num++;
  if (ret_val == 0) {
    /*If process is not alive anymore, free his taskmonitor*/
    kref_put(&tm->kref_tm, release_taskmonitor);
    return 0;
  }
  kref_init(&new->kref);
  new->pid = tm->ts_monitored->pid;
  mutex_lock(&tm->mx);
  list_add_tail(&new->list, &tm->head);
  tm->count++;
  mutex_unlock(&tm->mx);
  return 1;
}

static int monitor_fn(void *arg) {
  struct list_head *it;
  struct task_monitor *tm;
  while (!kthread_should_stop()) {
    mutex_lock(mtx_taskmonitor);
    list_for_each(it, &head_taskmonitor) {
      tm = container_of(it, struct task_monitor, tasks);
      save_sample(tm);
    }
    mutex_unlock(mtx_taskmonitor);
    ssleep(10);
  }
  return 0;
}

static ssize_t store_cmd(struct kobject *kobj, struct kobj_attribute *attr,
                         const char *buf, size_t count) {
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
    __ATTR(taskmonitor, 0600, NULL, store_cmd);

static int __init tm_init(void) {
  int ret_val = sysfs_create_file(kernel_kobj, &system_t_monitor_attr.attr);
  if (ret_val)
    goto err_sysfs;

  ktmonitor_thread = kthread_run(monitor_fn, NULL, "monitor_fn");

  /*Register to the shrinker*/
  if (register_shrinker(&taskmonitor_shrinker, "taskmonitor_shrinker"))
    goto err_register_shrinker;

  /*creating our slab*/
  taskmonitor_cache = KMEM_CACHE(task_sample, 0);
  if (!taskmonitor_cache)
    goto err_taskmonitor_cache;

  /*Creating the mempool of min 3 space (sure to be allocated 3 struct sample
   * space over the slab*/
  taskmonitor_mempool = mempool_create_slab_pool(3, taskmonitor_cache);
  if (!taskmonitor_mempool)
    goto err_mempool;

  /*Creating the debugfs file, rooted to the debufs directory*/
  taskmonitor_debugfs =
      debugfs_create_file("taskmonitor", 0644, NULL, NULL, &taskmonitor_fops);
  if (taskmonitor_debugfs == 0) {
    goto err_debugfs;
  }

  taskmonitor_cache_tm = KMEM_CACHE(task_monitor, 0);
  if (!taskmonitor_cache_tm)
    goto err_taskmonitor_cache_tm;

  taskmonitor_mempool_tm = mempool_create_slab_pool(3, taskmonitor_cache_tm);
  if (!taskmonitor_mempool_tm)
    goto err_taskmonitor_mempool_tm;

  mtx_taskmonitor = kzalloc(sizeof(struct mutex), GFP_KERNEL);
  mutex_init(mtx_taskmonitor);

  return 0;

err_taskmonitor_mempool_tm:
  kmem_cache_destroy(taskmonitor_cache_tm);
err_taskmonitor_cache_tm:
  debugfs_remove(taskmonitor_debugfs);
err_debugfs:
  mempool_destroy(taskmonitor_mempool);
err_mempool:
  kmem_cache_destroy(taskmonitor_cache);
err_taskmonitor_cache:
  unregister_shrinker(&taskmonitor_shrinker);
err_register_shrinker:
  kthread_stop(ktmonitor_thread);
  sysfs_remove_file(kernel_kobj, &system_t_monitor_attr.attr);
err_sysfs:
  return -EINVAL;
}

static void __exit tm_exit(void) {
  if (ktmonitor_thread) {
    kthread_stop(ktmonitor_thread);
  }

  struct task_monitor *it, *tmp = NULL;
  mutex_lock(mtx_taskmonitor);
  list_for_each_entry_safe(it, tmp, &head_taskmonitor, tasks) {
    kref_put(&it->kref_tm, release_taskmonitor);
  }
  mutex_unlock(mtx_taskmonitor);
  kfree(mtx_taskmonitor);
  sysfs_remove_file(kernel_kobj, &system_t_monitor_attr.attr);
  unregister_shrinker(&taskmonitor_shrinker);
  kmem_cache_destroy(taskmonitor_cache);
  mempool_destroy(taskmonitor_mempool);
  mempool_destroy(taskmonitor_mempool_tm);
  kmem_cache_destroy(taskmonitor_cache_tm);
  debugfs_remove(taskmonitor_debugfs);
  pr_info("Task Monitor module unloaded\n");
  return;
}
module_init(tm_init);
module_exit(tm_exit);
