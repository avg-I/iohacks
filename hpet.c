#include <stdio.h>
#include <fcntl.h>

#include <unistd.h>
#include <sys/mman.h>

int main()
{
  int fd;
  void *v;
  size_t length = 0x1000;

  if((fd = open("/dev/mem", O_RDWR)) == -1) {
    perror("dev/mem");
    return (1);
  }

  v = mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0xfed00000);
  fprintf(stderr, "mmap returns %p\n", v);
      
  *(volatile unsigned char *)(v + 0x20) = 0xff; 
  return (0);
}
