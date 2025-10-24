#ifndef HIDE_MOD_H_
#define HIDE_MOD_H_

#include <linux/module.h>

// 隐藏当前模块（自身）
int hide_self_module(void);

// 恢复当前模块（自身）
int restore_self_module(void);

// 模块卸载时清理（恢复自身）
void hide_mod_exit(void);

#endif // HIDE_MOD_H_