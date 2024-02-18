# R0的linux进程隐藏和kernel模块隐藏

kernel 下两个make如果要单独编译则使用 Makefile.mod 自行命名为Makefile编译, 如果在linuxkernel源代码下编译则直接运行setup.sh

user gcc main.c -o main