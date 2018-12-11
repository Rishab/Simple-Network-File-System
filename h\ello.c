#define FUSE_USE_VERSION 30

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<time.h>
#include<string.h>
#include<fuseh>

#include<sys/types.h>

/* takes two parameters, the path and a structure that contains all the information of a particular file */
static int fuse_getattr(const chr * path, struct stat * st) {
	
	printf("getattr called\n");
	printf("Attributes of %s requested\n", path);
	st->st_uid = getuid(); //user ud
	st->st_gid = getgid(); //group id
	st->st_atime = time(NULL); //last access time
	st->st_mtime = time(NULL); //last modification time
	
	if (strcmp(path, "/") == 0) {
		st->st_mode = S_IFDIR | 0755; //mode of the file, directory, readable and executed by others but only writeable by user
	  	st->st_nlink = 2; //number of links to the files
	}
	else {
		st->st_mode = S_IFREG | 0644; //mode of the file, regular, readable by all but only writeable by user
		st->st_nlink = 1;
		st->st_size = 1024; //file size
	}
	
	return 0;
}


