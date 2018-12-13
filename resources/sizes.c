#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

int char_size = 0;

int charsize (int n) {
	
	if (n == 4) {
		return 10;
	}

	if (n == 8) {
		return 20;
	}

	printf("WARNING: unaccounted-for value in charsize: %d\n", n);
	return 0;
}


int main(int argc, char **argv) {

	int total = 0;

	int arg = sizeof(dev_t);
	total += arg;
	char_size += charsize(arg);
	printf("dev_t: %d\nrunning total: %d\nrunning char_size: %d\n", arg, total, char_size);

	arg = sizeof(ino_t);
	total += arg;
	char_size += charsize(arg);
	printf("ino_t: %d\nrunning total: %d\nrunning char_size: %d\n", arg, total, char_size);

	arg = sizeof(mode_t);
	total += arg;
	char_size += charsize(arg);
	printf("mode_t: %d\nrunning total: %d\nrunning char_size: %d\n", arg, total, char_size);

	arg = sizeof(nlink_t);
	total += arg;
	char_size += charsize(arg);
	printf("nlink_t: %d\nrunning total: %d\nrunning char_size: %d\n", arg, total, char_size);

	arg = sizeof(uid_t);
	total += arg;
	char_size += charsize(arg);
	printf("uid_t: %d\nrunning total: %d\nrunning char_size: %d\n", arg, total, char_size);

	arg = sizeof(gid_t);
	total += arg;
	char_size += charsize(arg);
	printf("gid_t: %d\nrunning total: %d\nrunning char_size: %d\n", arg, total, char_size);

	arg = sizeof(dev_t);
	total += arg;
	char_size += charsize(arg);
	printf("dev_t: %d\nrunning total: %d\nrunning char_size: %d\n", arg, total, char_size);

	arg = sizeof(off_t);
	total += arg;
	char_size += charsize(arg);
	printf("off_t: %d\nrunning total: %d\nrunning char_size: %d\n", arg, total, char_size);

	arg = sizeof(time_t);
	total += arg;
	char_size += charsize(arg);
	printf("time_t (1): %d\nrunning total: %d\nrunning char_size: %d\n", arg, total, char_size);

	arg = sizeof(time_t);
	total += arg;
	char_size += charsize(arg);
	printf("time_t (2): %d\nrunning total: %d\nrunning char_size: %d\n", arg, total, char_size);

	arg = sizeof(time_t);
	total += arg;
	char_size += charsize(arg);
	printf("time_t (3): %d\nrunning total: %d\nrunning char_size: %d\n", arg, total, char_size);

	arg = sizeof(blksize_t);
	total += arg;
	char_size += charsize(arg);
	printf("blksize_t: %d\nrunning total: %d\nrunning char_size: %d\n", arg, total, char_size);

	arg = sizeof(blkcnt_t);
	total += arg;
	char_size += charsize(arg);
	printf("blkcnt_t: %d\ngrand total: %d\nrgrand total char_size: %d\n", arg, total, char_size);

	printf("size of stat structure: %d\n", sizeof(struct stat));

	return 0;
}
