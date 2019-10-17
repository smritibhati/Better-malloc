#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>

#ifndef SAFE_FACTOR
#define SAFE_FACTOR 4
#endif

struct chunk{
  void *idx;
  size_t sz;
};

struct list_heads{
  struct chunk *start_list, *end_list;
  size_t count;
  size_t vol;
} *usedl, *freel;

void *SLAB;

int Mem_Init(int SZ){

  // Disallowing any negative range requests
  if(SZ <= 0) return -1;

  // File descriptor to be used for /dev/zero
  int zerofd;
  if(zerofd = open("/dev/zero", O_RDWR) < 0){
    return -1;
    exit(1);
  }

  // Trying to MAP requested size of memory
  if(SLAB = mmap(0, SAFE_FACTOR * SZ, PROT_READ | PROT_WRITE, MAP_PRIVATE, zerofd, 0) == MAP_FAILED){
    return -1;
    exit(1);
  }

  // Initialize list heads ( used, free )
  // Give proper count and usage values
  usedl = (struct list_heads *) ( SLAB + SZ );
  usedl->count = 0;
  usedl->vol = 0;

  freel = (struct list_heads *) ( SLAB + SZ + sizeof(struct list_heads));
  freel->count = -1;
  freel->vol = SZ;

  // clean close for zerofd
  close(zerofd);
  return 1;
}

int main(int argc, char *argv[]){
  Mem_Init(50000);
}
