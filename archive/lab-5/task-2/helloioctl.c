#include <linux/fs.h>
#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sysfs.h>
#include <linux/uaccess.h>

#include "helloioctl.h"

#define READ_CMD _IOR(MAGIC_NUM, 0, char *)
#define WRT_CMD _IOW(MAGIC_NUM, 1, char *)

int major;
char *mess = "Hello ioctl!\n";

MODULE_DESCRIPTION("Hello ioctl");
MODULE_AUTHOR("Eros Chan");
MODULE_LICENSE("GPL");

static long etx_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
  switch (cmd) {
  case READ_CMD:
    if (copy_to_user((void *)arg, mess, strlen(mess)) != 0) {
      pr_err("ioctl cannot copy!\n");
      return -ENOTTY;
    }
    break;
  default:
    pr_err("Command not found!\n");
    return -ENOTTY;
  }

  return 0;
}

struct file_operations fops = {.unlocked_ioctl = etx_ioctl};
void print_hello(void) { pr_info("Hello ioctl!\n"); }

static int __init helloioctl_init(void) {
  major = register_chrdev(0, "hello", &fops);
  pr_info("major=%d\n", major);
  return 0;
}

static void __exit helloioctl_exit(void) { unregister_chrdev(major, "hello"); }

module_init(helloioctl_init);
module_exit(helloioctl_exit);
