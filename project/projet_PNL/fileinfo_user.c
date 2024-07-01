// SPDX-License-Identifier: GPL-2.0
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#define CMD _IOR('N', 0, char *)

int main(int argc, char **argv)
{
	if (argc < 2) {
		printf("Usage: %s <file_name>", argv[1]);
		return -1;
	}

	char *file = argv[1];
	int device_fd = open("/dev/fileinfo", O_RDWR);
	int file_fd = open(file, O_RDWR);
	int ret = ioctl(device_fd, CMD, file_fd);

	if (ret)
		perror("Failed ioctl");

	close(file_fd);
	close(device_fd);

	return 0;
}
