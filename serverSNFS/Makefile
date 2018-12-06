CC := gcc
CFLAGS := -g -Wall

all: serverSNFS

serverSNFS: serverSNFS.c
	$(CC) $(CFLAGS) serverSNFS.c -o serverSNFS -lpthread

clean:
	rm -f serverSNFS
