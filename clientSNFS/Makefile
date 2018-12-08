CC := gcc
CFLAGS := -g -Wall -D_FILE_OFFSET_BITS=64

all: clientSNFS

clientSNFS: clientSNFS.c
	$(CC) $(CFLAGS) clientSNFS.c -o clientSNFS

clean:
	rm -f clientSNFS
