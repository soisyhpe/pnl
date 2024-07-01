

#include "linux/dcache.h"
#include "linux/fs.h"
#include "linux/list_bl.h"
#include "linux/list_nulls.h"
#include "linux/module.h"
#include "linux/proc_fs.h"
#include "linux/seq_file.h"
#include "linux/slab.h"
#include "linux/types.h"

MODULE_AUTHOR("Harena RAKOTONDRATSIMA");
MODULE_DESCRIPTION("Print dentry cache address  and it is size");
MODULE_LICENSE("GPL");

extern unsigned int d_hash_shift;

static ssize_t weasel_whoiam_read(struct file *file, char __user *buf,
                                  size_t count, loff_t *ppos) {
  const char *tmp = "I' m a weasel\n";
  return simple_read_from_buffer(buf, count, ppos, tmp, strlen(tmp));
}

static const struct proc_ops whoiam_fops = {
    .proc_open = simple_open,
    .proc_read = weasel_whoiam_read,
};

static struct proc_dir_entry *proc_weasel;
static struct proc_dir_entry *proc_weasel_whoiam;
static struct proc_dir_entry *proc_weasel_dcache;
static struct proc_dir_entry *proc_weasel_pwd;
static int dcache_show(struct seq_file *seq, void *offset) {
  struct hlist_bl_head *bucket;
  struct dentry *dentry;
  struct hlist_bl_node *it;
  int max = 0;
  int max_init = 0;
  unsigned long size = 1 << d_hash_shift;
  int cpt = 0;

  for (int i = 0; i < size; i++) {
    bucket = &dentry_hashtable[i];
    hlist_bl_for_each_entry(dentry, it, bucket, d_hash) {
      cpt++;
      max++;
      seq_dentry(seq, dentry, "\t\n");
      seq_printf(seq, " \n");
    }

    if (max_init < max)
      max_init = max;

    max = 0;
  }
  return 0;
}

static int dcache_open(struct inode *inode, struct file *file) {
  return single_open(file, dcache_show, NULL);
}
static int pwd_show(struct seq_file *seq, void *offset) {
  struct hlist_bl_head *bucket;
  struct dentry *dentry;
  struct hlist_bl_node *it;
  unsigned long size = 1 << d_hash_shift;

  for (int i = 0; i < size; i++) {
    bucket = &dentry_hashtable[i];
    hlist_bl_for_each_entry(dentry, it, bucket, d_hash) {
      if (dentry->d_inode)
        continue;
      seq_dentry(seq, dentry, "\t\n");
      seq_printf(seq, " \n");
    }
  }
  return 0;
}

static int pwd_open(struct inode *inode, struct file *file) {
  return single_open(file, pwd_show, NULL);
}
static const struct proc_ops dcache_fops = {
    .proc_open = dcache_open,
    .proc_read = seq_read,
};

static const struct proc_ops pwd_fops = {
    .proc_open = pwd_open,
    .proc_read = seq_read,
};
static int __init weasel_init(void) {
  proc_weasel = proc_mkdir("weasel", NULL);
  proc_weasel_whoiam = proc_create("whoiam", 0, proc_weasel, &whoiam_fops);
  proc_weasel_dcache = proc_create("dcache", 0, proc_weasel, &dcache_fops);
  proc_weasel_pwd = proc_create("pwd", 0, proc_weasel, &pwd_fops);
  return 0;
}

static void __exit weasel_exit(void) {
  remove_proc_entry("pwd", proc_weasel);
  remove_proc_entry("dcache", proc_weasel);
  remove_proc_entry("whoiam", proc_weasel);
  remove_proc_subtree("weasel", NULL);
  pr_info("Exiting module weasel\n");
}

module_init(weasel_init);
module_exit(weasel_exit);
