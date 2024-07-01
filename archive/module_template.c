#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

MODULE_DESCRIPTION("Hello ioctl");
MODULE_AUTHOR("Eros Chan");
MODULE_LICENSE("GPL");

struct file_operations fops = {};

static int __init module_init(void) { return 0; }

static void __exit module_exit(void) {}

module_init(module_init);
module_exit(module_exit);
