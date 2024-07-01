#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#define READ_CMD _IOR('N', 0, char*)
#define WRT_CMD _IOW('N', 1, char*)


int main() 
{
  int fd = open("/dev/hello",O_RDWR);

  char mess[100];
  if (ioctl(fd, READ_CMD,mess)) {
    perror("Failed ioctl hello\n");
    return -1;
  }
  printf("%s\n",mess);
  //close(fd);
  return 0;
}
