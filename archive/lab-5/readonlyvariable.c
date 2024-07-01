#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>

MODULE_DESCRIPTION("Hello sysfs");
MODULE_AUTHOR("Eros Chan");
MODULE_LICENSE("GPL");

static int system_enabled;
static u64 clock;

static ssize_t system_state_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
  return snprintf(buf, PAGE_SIZE, "The system is %srunning\n", system_enabled ? "" : "not ");
}

static ssize_t clock_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
  return snprintf(buf, PAGE_SIZE, "%llu\n", clock++);
}

static ssize_t clock_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {
  u64 val;
  int rc = sscanf(buf, "%llu", &val);
  if (rc != 1 || rc < 0)
    return -EINVAL;

  clock = val;
  return count;
}

static struct kobj_attribute system_state_attribute = __ATTR(system_enabled, 0400, system_state_show, NULL);
static struct kobj_attribute clock_attribute = __ATTR(clock, 0600, clock_show, clock_store);

static struct attribute *attrs[] = {
  &system_state_attribute.attr,
  &clock_attribute.attr,
  NULL
};

static struct attribute_group attr_grp = { .attrs = attrs };

static struct kobject *my_state_kobj;

static int __init my_state_init(void) {
  // pr_info("Hello world!\n");
  
  int retval;

  my_state_kobj = kobject_create_and_add("my_state", kernel_kobj);
  if (!my_state_kobj)
    goto error_init_1;

  retval = sysfs_create_group(my_state_kobj, &attr_grp);
  if (retval)
    goto error_init_2;

  return 0;

error_init_2:
  kobject_put(my_state_kobj);
error_init_1:
  return -ENOMEM;
}

module_init(my_state_init);

static void __exit my_state_exit(void) {
  // pr_info("Goodbye world...\n");

  kobject_put(my_state_kobj);
}

module_exit(my_state_exit);
