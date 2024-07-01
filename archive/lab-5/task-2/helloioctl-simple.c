#include <linux/fs.h>
#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sysfs.h>

MODULE_DESCRIPTION("Hello ioctl");
MODULE_AUTHOR("Eros Chan");
MODULE_LICENSE("GPL");

static int major;

static struct file_operations fops = {};

static int __init helloioctl_init(void) {
  major = register_chrdev(0, "hello", &fops);
  pr_info("major=%d\n", major);
  return 0;
}

module_init(helloioctl_init);

static void __exit helloioctl_exit(void) { unregister_chrdev(major, "hello"); }

module_exit(helloioctl_exit);
