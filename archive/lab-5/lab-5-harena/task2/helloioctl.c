#include "asm-generic/errno-base.h"
#include "linux/kobject.h"
#include "linux/parser.h"
#include "linux/printk.h"
#include "linux/sysctl.h"
#include "linux/types.h"
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h> // Module metadata
#include <linux/sysfs.h>
#include <linux/uaccess.h>

#include "helloioctl.h"

#define READ_CMD _IOR(MAGIC_NUM, 0, char *)
#define WRT_CMD _IOW(MAGIC_NUM, 1, char *)
char *mess = "Hello IOCTL!\n";
char buff[100];

MODULE_AUTHOR("Harena RAKOTONDRATSIMA");
MODULE_DESCRIPTION("Hello world module");
MODULE_LICENSE("GPL");
unsigned int maj;

static long etx_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {

  switch (cmd) {
  case READ_CMD:
    if (copy_to_user((void *)arg, mess, strlen(mess)) != 0) {
      pr_err("Ioctl cannot copy!!\n");
      return -EINVAL;
    }

    break;
  default:
    pr_err("Command not found ! \n");
    return EINVAL;
  }

  return 0;
}

struct file_operations fops = {.unlocked_ioctl = etx_ioctl};
void print_hello(void) { pr_info("Hello ioctl!\n"); }

static int __init helloioctl_init(void) {
  maj = register_chrdev(0, "hello", &fops);
  pr_info("major : %d\n", maj);
  return 0;
}

static void __exit helloioctl_exit(void) { unregister_chrdev(maj, "Hello"); }
module_init(helloioctl_init);
module_exit(helloioctl_exit);
