#include "linux/kobject.h"
#include "linux/parser.h"
#include "linux/types.h"
#include <linux/init.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include  <linux/proc_fs.h>// Module metadata
#include <linux/kernel.h>
#include <linux/sysfs.h>
MODULE_AUTHOR("Harena RAKOTONDRATSIMA");
MODULE_DESCRIPTION("Hello world module");
MODULE_LICENSE("GPL");
static ssize_t show_hello(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
  return snprintf(buf, PAGE_SIZE, "Hello sysfs!\n");
}
static struct kobj_attribute system_hellosysfs_attribute = __ATTR(hellosysfs, 0400, show_hello, NULL);
static struct kobject *my_hellosysfs;

static int __init hellosysfs_init(void)
{
  int retval;
  my_hellosysfs = kobject_create_and_add("myhellosysfs", kernel_kobj);
  if (!my_hellosysfs)
    goto error_init_1;

  retval = sysfs_create_file(my_hellosysfs, &system_hellosysfs_attribute.attr);
  if( retval)
    goto error_init_2;

  return 0;

  error_init_2 :
    kobject_put(my_hellosysfs);
  error_init_1 :
    return -ENOMEM;  
}


static void __exit hellosysfs_close(void)
{

  kobject_put(my_hellosysfs);  
}

module_init(hellosysfs_init);
module_exit(hellosysfs_close);
