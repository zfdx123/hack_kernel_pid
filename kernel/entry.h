#ifndef ENTRY_H_
#define ENTRY_H_

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

// 仅保留必要声明
int dispatch_open(struct inode *node, struct file *file);
int dispatch_close(struct inode *node, struct file *file);
long dispatch_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
int __init module_entry(void);
void __exit exit_module(void);

#endif // ENTRY_H_