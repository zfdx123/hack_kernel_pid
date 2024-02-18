#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

enum OPERATIONS
{
	OP_HIDE_PID = 0x800,
	OP_RED_PID = 0x801,

	OP_HIDE_MOD = 0X8002,
	OP_RED_MOD = 0x8003,
};

#define MISC_NAME "/dev/HIDE"

int main()
{
	int fd = open(MISC_NAME, O_RDWR);
	if (fd < 0)
	{
		perror("open failure\n");
		return -1;
	}

	int retvalue = ioctl(fd, OP_RED_MOD, 0);
	if (0 != retvalue)
	{
		perror("ioctl failure\n");
	}

	close(fd);
	return 0;
}