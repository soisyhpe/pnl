#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>

MODULE_DESCRIPTION("Hello world module");
MODULE_AUTHOR("Eros Chan");
MODULE_LICENSE("GPL");

static int __init hello_init(void) {
  pr_info("Hello world!\n");
  return 0;
}

module_init(hello_init);

static void __exit hello_exit(void) {
  pr_info("Goodbye world...\n");
}

module_exit(hello_exit);
