# PNL : LAB-7 : Virtual File System

---

## Tâche 1 :

---

> **Question 1**

> **Question 2**

Il faut retirer le `static` et exporter le symbole.

> **Question 3**

`diff -Naru original_file updated_file > my.patch`

## Tâche 2 : Un cache pas si privé

---

> **Question 1**

La taille de la hash table : 

```c
numentries = 1UL << *_hash_shift;
```

Unsigned long * 2 ^(hash_shift)

1 >> sizeof(int) * 8 - d_hash_shift

> **Question 2**

> **Question 5**
> In order to export the dentry cache content to user space, implement a procfs file `/proc/weasel/dcache` that lists the content of the dentry cache. You can implement this with a simplified version of seq files. Instead of implementing a `struct seq_operations`, you can just use the single_open() function in your open function in the `struct proc_ops` and set the other function pointers properly. You can find multiple examples in the kernel sources.

> [!info] To get the full path of a dentry, you can look up exported functions in `fs/d_path.c`.

```c
static struct proc_dir_entry *proc_weasel_dcache;

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
```

Sources:
- [The seq_file interface](https://docs.kernel.org/filesystems/seq_file.html)

> **Question 6**
> In the terminal of the VM, try to execute a non-existent program, e.g., foo and look it up in your `/proc/weasel/dcache` file. From this output, infer the content of your `$PATH` environment variable. Why are these errors recorded in the dentry cache?

Ces erreurs sont enregistrées dans le dentry cache pour que le kernel n'ait pas à re-calculer leurs chemins.

> **Question 7**
> Sometimes, when a user tries to authenticate themselves with a command such as su, they might type their password a second time directly in case of failure. This leads the shell to interpret this as a command.
> Add a new procfs file in your module, `/proc/weasel/pwd`, that only display the list of commands that were not found in the `$PATH`.

```c
static struct proc_dir_entry *proc_weasel_pwd;

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
```

## Tâche 3 : Rootkit - Hide a process (naive)

---

> **Question 1**
> Implement the skeleton of your module that takes as a parameter the PID of the process to hide.


