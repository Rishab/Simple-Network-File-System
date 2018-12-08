#define FUSE_USE_VERSION 30

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<time.h>
#include<string.h>
#include<fuseh>

#include<sys/types.h>

static int fuse_getattr(const chr * path, struct stat * st) {
  st->st_uid = getuid();
  st->st_gid = getgid();
  st->st_atime = time(NULL);
  st->st_mtime = time(NULL);

  
}
