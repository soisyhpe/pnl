#include "linux/dcache.h"
#include "linux/list_bl.h"
#include "linux/list_nulls.h"
#include "linux/module.h"
#include "linux/proc_fs.h"
#include "linux/seq_file.h"

MODULE_DESCRIPTION("weasel");
MODULE_AUTHOR("Eros Chan");
MODULE_LICENSE("GPL");

extern unsigned int d_hash_shift;
extern struct hlist_bl_head *dentry_hashtable;

static struct proc_dir_entry *proc_parent;
static struct proc_dir_entry *proc_parent_firstchild;

static int weasel_pwd_show(struct seq_file *m, void *v) {
  unsigned long size;
  struct hlist_bl_head *bucket;
  struct hlist_bl_node *it;
  struct dentry *dentry;

  size = 1 << (32 - d_hash_shift);

  hlist_bl_lock(dentry_hashtable);
  for (bucket = dentry_hashtable; bucket < dentry_hashtable + size; bucket++) {
    hlist_bl_for_each_entry(dentry, it, bucket, d_hash) {
      if (dentry->d_inode != NULL)
        continue;
      seq_dentry(m, dentry, "\t\n");
      seq_printf(m, "\n");
    }
  }
  hlist_bl_unlock(dentry_hashtable);
  return 0;
}

static int weasel_pwd_open(struct inode *inode, struct file *file) {
  return single_open(file, weasel_pwd_show, NULL);
}

static const struct proc_ops pwd_ops = {
    .proc_open = weasel_pwd_open,
    .proc_read = seq_read,
};

static int weasel_dcache_show(struct seq_file *m, void *v) {
  int i;
  unsigned long size;
  struct hlist_bl_head *bucket;
  struct hlist_bl_node *it;
  struct dentry *dentry;

  size = 1 << (32 - d_hash_shift);

  hlist_bl_lock(dentry_hashtable);
  for (i = 0; i < size; i++) {
    bucket = &dentry_hashtable[i];

    hlist_bl_for_each_entry(dentry, it, bucket, d_hash) {
      seq_dentry(m, dentry, "\t\n");
      seq_printf(m, "\n");
    }
  }
  hlist_bl_unlock(dentry_hashtable);
  return 0;
}

static int weasel_dcache_open(struct inode *inode, struct file *file) {
  return single_open(file, weasel_dcache_show, NULL);
}

static const struct proc_ops dcache_ops = {
    .proc_open = weasel_dcache_open,
    .proc_read = seq_read,
    .proc_release = single_release,
};

static ssize_t weasel_whoami_read(struct file *file, char __user *buf,
                                  size_t count, loff_t *ppos) {
  const char *tmp = "I'm a weasel!\n";
  return simple_read_from_buffer(buf, count, ppos, tmp, strlen(tmp));
}

static const struct proc_ops ops = {
    .proc_read = weasel_whoami_read,
};

int proc_create_weasel(void) {
  proc_parent = proc_mkdir("weasel", NULL);

  if (!proc_parent) {
    pr_err("Error: Unable to mkdir weasel!");
    return -ENOMEM;
  }

  proc_parent_firstchild = proc_create("whoami", 0, proc_parent, &ops);

  if (!proc_parent_firstchild) {
    pr_err("Error: Unable to create entry whoami!");
    return -ENOMEM;
  }

  proc_parent_firstchild = proc_create("dcache", 0, proc_parent, &dcache_ops);

  if (!proc_parent_firstchild) {
    pr_err("Error: Unable to create entry dcache!");
    return -ENOMEM;
  }

  proc_weasel_pwd = proc_create("pwd", 0, proc_parent, &pwd_ops);

  if (!proc_weasel_pwd) {
    pr_err("Error: Unable to create entry pwd!");
    return -ENOMEM;
  }

  return 0;
}

void proc_remove_weasel(void) {
  remove_proc_entry("dcache", proc_parent_firstchild);
  remove_proc_entry("whoami", proc_parent_firstchild);
  remove_proc_entry("weasel", NULL);
}

static int __init weasel_init(void) {
  proc_create_weasel();

  return 0;
}
module_init(weasel_init);

static void __exit weasel_exit(void) {
  pr_info("Exiting weasel\n");
  proc_remove_weasel();
}
module_exit(weasel_exit);
