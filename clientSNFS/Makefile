CC := gcc
CFLAGS := -g -Wall -fsanitize=address -D_FILE_OFFSET_BITS=64 `pkg-config fuse --cflags --libs`

all: clientSNFS

clientSNFS: clientSNFS.c
	$(CC) $(CFLAGS) clientSNFS.c -o clientSNFS

clean:
	rm -f clientSNFS
