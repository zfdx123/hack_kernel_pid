CONFIG_MODULE_SIG=n

# 目标模块名（根据实际文件调整）
obj-m += hide.o
hide-objs := entry.o hide_pid.o hide_mod.o  # 所有源文件，用空格分隔

# 内核源码路径（自动获取当前运行内核的头文件）
KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)


# 编译规则
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

# 清理编译产物
clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
	rm -f *.ko *.mod.* *.o *.symvers *.order