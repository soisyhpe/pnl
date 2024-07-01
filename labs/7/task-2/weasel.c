#include "linux/dcache.h"
#include "linux/list_bl.h"
#include "linux/list_nulls.h"
#include "linux/module.h"
// #include <linux/init.h>
// #include <linux/kernel.h>
// #include <linux/module.h>

MODULE_DESCRIPTION("weasel");
MODULE_AUTHOR("Eros Chan");
MODULE_LICENSE("GPL");

extern unsigned int d_hash_shift;
extern struct hlist_bl_head *dentry_hashtable;

static int __init weasel_init(void) {
  int i, buckets_count, bucket_entry_count, max_bucket_entry_count;
  unsigned long size;
  struct hlist_bl_head *bucket;
  struct hlist_bl_node *it;
  struct dentry *dentry;

  buckets_count = 0;
  max_bucket_entry_count = 0;
  // size = 1 << d_hash_shift;
  pr_info("d_hash_shift=%du\n", d_hash_shift);
  size = 1 << (32 - d_hash_shift); // 32 - d_hash_shift ou d_hash_shift ?
  

  hlist_bl_lock(dentry_hashtable);
  for (i = 0; i < size; i++) {
    bucket = &dentry_hashtable[i];
    bucket_entry_count = 0;

    hlist_bl_for_each_entry(dentry, it, bucket, d_hash) {
      buckets_count++;
      bucket_entry_count++;
    }

    if (bucket_entry_count > max_bucket_entry_count)
      max_bucket_entry_count = bucket_entry_count;
  }
  hlist_bl_unlock(dentry_hashtable);

  pr_info("Number of buckets of the hash table: %lu\n", size);
  pr_info("Address of dentry cache: %p\n", dentry_hashtable);
  pr_info("Number of entries %d, Length of the longest bucket %d\n",
          buckets_count, max_bucket_entry_count);

  return 0;
}
module_init(weasel_init);

static void __exit weasel_exit(void) { pr_info("Exiting weasel\n"); }
module_exit(weasel_exit);
