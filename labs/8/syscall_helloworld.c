#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

MODULE_DESCRIPTION("Hello world system call");
MODULE_AUTHOR("Eros Chan");
MODULE_LICENSE("GPL");

static int __init module_init(void) {

  return 0;
}
module_init(module_init);

static void __exit module_exit(void) {}
module_exit(module_exit);
