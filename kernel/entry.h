#ifndef ENTRY_H_
#define ENTRY_H_

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h> // 包含文件操作结构体的定义
#include <linux/uaccess.h> // 包含用户空间访问的相关函数

// 函数原型声明
void module_hide(void);
void module_show(void);
int dispatch_open(struct inode *node, struct file *file);
int dispatch_close(struct inode *node, struct file *file);
long dispatch_ioctl(struct file *const file, unsigned int const cmd, unsigned long const arg);
int __init module_entry(void);
void __exit exit_module(void);

#endif // ENTRY_H_
