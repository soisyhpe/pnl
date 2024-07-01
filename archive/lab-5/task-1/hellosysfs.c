#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/sysfs.h>

MODULE_DESCRIPTION("Hello sysfs");
MODULE_AUTHOR("Eros Chan");
MODULE_LICENSE("GPL");

static char state[PAGE_SIZE];

static ssize_t state_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
  return snprintf(buf, PAGE_SIZE, "Hello %s!\n", state);
}

static ssize_t state_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {
  snprintf(state, PAGE_SIZE, buf);
  return count;
}

static struct kobj_attribute state_attribute = __ATTR(state, 0600, state_show, state_store);
static struct kobject *my_state_kobj;

static int __init my_state_init(void) {
  int retval;
  //my_state_kobj = kobject_create_and_add("my_state", kernel_kobj);
  //if (!my_state_kobj)
    //goto error_init_1;
  retval = sysfs_create_file(kernel_kobj, &state_attribute.attr);
  if (retval)
    //goto error_init_2;
    kobject_put(my_state_kobj);
  return 0;
//error_init_2:
  //kobject_put(my_state_kobj);
//error_init_1:
  //return -ENOMEM;
}

module_init(my_state_init);

static void __exit my_state_exit(void) {
  kobject_put(my_state_kobj);
}

module_exit(my_state_exit);
