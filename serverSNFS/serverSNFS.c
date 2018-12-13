#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUF_SIZE 10

/* For Mohit: not the best way of finding the number of files and directories. I will figure
 * out a better way soon
 */

int port = 0;

char *mount_path = NULL;

/*
 * Takes in two malloc'ed strings, and concatenates them str1 | str2.
 * If either str1 or str2 is NULL, it returns the non-NULL one.
 * If both str1 and str2 are NULL, it returns NULL.
 * Also frees the memory for str1 and str2 assuming they are not NULL.
 */
char *strcat_dynamic(char *str1, char *str2, int free_first)
{
	if (str1 == NULL && str2 == NULL) {
		return NULL;
	}

	if (str1 == NULL) {
		return str2;
	}

	if (str2 == NULL) {
		return str1;
	}

	// Total length of new string, plus 1 for a null terminator.
	int concat_len = strlen(str1) + strlen(str2) + 1;
	char *concat_str = (char *) malloc(sizeof(char) * concat_len);
	memset(concat_str, 0, concat_len);
	strncpy(concat_str, str1, strlen(str1));
	strncpy(concat_str + strlen(str1), str2, strlen(str2));

	// Free memory for input strings.
    if (free_first) {
        free(str1);
    }
	//free(str2);

	return concat_str;
}

int num_digits (int n) {
	int digits = 0;
//	printf("%d ", n);

	while(n > 0) {
		n /= 10;
		digits++;
	}

//	printf("has %d digits\n", digits);
	return digits;
}

DIR * opendir_handler(char * file_path) {
	DIR * dir =  opendir(file_path);

	if (dir == NULL) {
		return NULL;
	}

	return dir;
}

int readdir_handle_length(char * file_path) {
	DIR * dir = opendir_handler(file_path);
	struct dirent * dp;

	int num_files = 0;
	int files_length = 0;

	while ((dp=readdir(dir)) != NULL) {
		//printf("File name: %s\n", dp->d_name);
		files_length += strlen(dp->d_name);
		num_files++;
	}

	printf("The length of the file stream is: %d\n", files_length);
    return files_length;
}

int readdir_handle_num_entries(char * file_path) {
	DIR * dir = opendir_handler(file_path);
	struct dirent * dp;

	int num_files = 0;

	while ((dp=readdir(dir)) != NULL) {
		//printf("File name: %s\n", dp->d_name);
		num_files++;
	}

	printf("The number of entries is: %d\n", num_files);
    return num_files;
}

char * readdir_handle_string(char * file_path, int stream_length, int num_files) {

	printf("handling string readdir...\n");

	DIR * dir =  opendir(file_path);
	struct dirent * dp;

	if (dir == NULL) {
		printf("dir is NULL\n");
		return NULL;
	}

	int num_files_size = num_digits(num_files);
	char *num_files_str = (char *) malloc(sizeof(char) * num_files_size + 1);
	//printf("num_files: value [%d], size [%d]\n", num_files, num_files_size + 1);

	int stream_len_size = num_digits(stream_length);

	int final_stream_len = stream_length + num_files + 2 + num_files_size + stream_len_size;
	int final_stream_len_size = num_digits(final_stream_len);
	char *final_stream_len_str = (char *) malloc(sizeof(char) * final_stream_len_size);

	char * files = (char *) malloc(sizeof(char) * final_stream_len);

	snprintf(num_files_str, num_files_size + 1, "%d\n", num_files);
	snprintf(final_stream_len_str, final_stream_len_size + 1, "%d\n", final_stream_len);

	printf("num_files_str: %s\nfinal_stream_len_str: %s\n", num_files_str, final_stream_len_str);

	int i;
	int j = 0;

	for (i = 0; i < final_stream_len_size; i++) {
		files[i] = final_stream_len_str[i];
		//printf("setting files[%d] to %c\n", i, stream_len_str[i]);
	}

	files[i] = ',';
	i++;

	//printf("files: [%s]\n", files);

	for (j = 0; j < num_files_size; j++) {
		files[i] = num_files_str[j];
		i++;
		//printf("setting files[%d] to %c\n", i, num_files_str[j]);
	}
	files[i] = ',';
	i++;
	//printf("files: [%s]\n", files);

	while ((dp=readdir(dir)) != NULL) {

		//printf("File name: %s, files is now \"%s\"\n", dp->d_name, files);
		for (j = 0; j < strlen(dp->d_name); j++) {
			//printf("dp->d_name's length is: %d\n", strlen(dp->d_name));
			files[i] = dp->d_name[j];
			i++;
		}
		files[i] = ',';
		i++;
	}

	printf("The files name is: %s\n", files);

	return files;
}


/*
 * Thread handler for a client's connection. Handles all requests for a client
 * until no more are sent. Pass in a pointer to the socket descriptor (for
 * reading and writing) in as arg.
 */
void *client_handler(void *arg)
{
	printf("In a new thread\n");
	int client_fd = *((int *) arg); //client file descriptor for socket
	char *buffer = (char *) malloc(sizeof(char) * BUF_SIZE); //buffer to hold client message
	char *partial_buffer = NULL;

	int read_len = read(client_fd, buffer, BUF_SIZE);

	int i = 0, j = 0;
	int num_bytes = 0;

	// parse the message sent by the client.
	if (read_len > 0) {
		while (isdigit(buffer[i])) {
			i++;
			j++;
		}
		char *temp = malloc(sizeof(char) * (j + 1));
		i = 0;
		while (i < j) {
			temp[i] = buffer[i];
			i++;
		}
		temp[i] = '\0';
		num_bytes = atoi(temp);

		if (num_bytes >= read_len) {
			// The command overflows the buffer, so we have to read in the rest
			// of the bytes.
			int remaining_bytes = num_bytes - strlen(buffer) + 1;
			partial_buffer = (char *) malloc(sizeof(char) * (remaining_bytes));
			memset(partial_buffer, 0, remaining_bytes);
			int new_read = read(client_fd, partial_buffer, remaining_bytes);
			buffer = strcat_dynamic(buffer, partial_buffer, 1);
		}
		printf("Command: %s\n", buffer);

		// Now that we have the whole command string, we can parse it to find
		// the operation type.

		char *size = strtok(buffer, ",");
		char *op_type = strtok(NULL, ",");

		/*
			Similar to stat(). Getattr retrieves the status of the file in the given file path.
			Returns 0 if successful and -1 if failed.
			Protocol: <number of bytes to read>,<getattr>,<path to file>
		*/
		if (strcmp(op_type, "getattr") == 0) {
            		/* getattr
             		* Gets file information.
             		* Response form: <return_code>,<struct stat>
             		*/
			printf("Got a getattr request\n");
			char *path = strtok(NULL, ",");
            		char *absolute_path = strcat_dynamic(mount_path, path, 0);
	    		printf("Path: %s\n", absolute_path);
			struct stat server_stats;
			int ret = stat(absolute_path, &server_stats);

			if (ret != 0) {
				perror("error in stat");
			}



            		int ret_str_size = 300;
            		char *ret_str = (char *) malloc(ret_str_size);


			memset(ret_str, 0, ret_str_size);
			snprintf(ret_str, 300, "%d,%d,%ld,%ld,%d,%ld,%d,%d,%ld,%ld,%ld,%ld,%ld,%ld,%ld", ret, errno,
				server_stats.st_dev, server_stats.st_ino, server_stats.st_mode, server_stats.st_nlink, server_stats.st_uid,
				server_stats.st_gid, server_stats.st_rdev, server_stats.st_size, server_stats.st_atime,
				server_stats.st_mtime, server_stats.st_ctime, server_stats.st_blksize, server_stats.st_blocks);

			printf("Result: %s\n", ret_str);

			write(client_fd, ret_str, ret_str_size);
			return NULL;
		}

		/*
			Truncate will reduce or expand the file to th specified number of bytes.
			If the file is larger than the specified bytes, then data is lost.
			Returns 0 if successful and -1 if failed.
			Protocol: <number of bytes to read>,<getattr>,<path to file>
		*/
		else if (strcmp(op_type, "truncate") == 0) {
			printf("Got a truncate request\n");
			char *path = strtok(NULL, ",");
			printf("Truncate File Path: %s\n", path);
			int offset = atoi(strtok(NULL, ","));
			printf("Truncate Offset: %d\n", offset);
			int result = truncate(path, offset);
			if (result != 0) {
				perror("Truncate error on the server\n");
			}

			char * result_str = (char *) malloc (sizeof(int) * 10);
			memset(result_str, 0, 10);
			snprintf(result_str, 10, "%d", result);
			write(client_fd, result_str, 10);

		}

		/*
			Open will open a file. If the file does not already exist, it will create it first.
			The file is opened with the permissions specified (mode and permissions)
			and at the path given.
			If successful, open returns the file descriptor of the opened file. Else, -1.
			Protocol: <number of bytes to read>,<open>,<path>,<flags>
		*/
		else if (strcmp(op_type, "open") == 0) {
			printf("Got an open request\n");
            char *open_path = strtok(NULL, ",");
            char *open_flags = strtok(NULL, ",");
            printf("Open path: %s\n", open_path);
            printf("Open flags: %s\n", open_flags);

            int flags = atoi(open_flags);

            int fd = open(open_path, flags);
            printf("Open result: %d\n", fd);

            char *result = (char *) malloc(sizeof(char) * 30);
            memset(result, 0, 30);
            snprintf(result, 30, "%d", fd);
            write(client_fd, result, 30);

		}

		/*
			Read will read the number of bytes requested by the client and store it in a buffer.
			Read has three additional arguments: the filepath, the number of bytes to read, and
			the offset from the beginning of the file.
			If successful, read will return the number of bytes read back to the client. Else, -1.
			Protocol: <number of bytes in the buffer>,<
		*/
		else if (strcmp(op_type, "read") == 0) {
			// read() has three additional arguments: the filepath, the number
			// of bytes to read, and the offset from the beginning of the file
			// where we should start reading from.
			printf("Got a read request\n");

            char *read_path = strtok(NULL, ",");
            char *size_str = strtok(NULL, ",");
            char *offset_str = strtok(NULL, ",");

            int size = atoi(size_str);
            int offset = atoi(offset_str);

            char *buffer = (char *) malloc(sizeof(char) * (size + 1));
            memset(buffer, 0, size + 1);

            int fd = open(read_path, O_RDWR);
            if (fd < 0) {
                char *result = (char *) malloc(sizeof(char) * (size + 20));
                memset(result, 0, size + 20);
                snprintf(result, size + 20, "%d", fd);
                write(client_fd, result, size + 20);
            } else {
                int read_res = pread(fd, buffer, size, offset);
                char *result = (char *) malloc(sizeof(char) * (size + 20));
                memset(result, 0, size + 20);
                snprintf(result, size + 20, "%d,%s", size + 20, buffer);
                write(client_fd, result, size + 20);
            }

        } else if (strcmp(op_type, "write") == 0) {
            // write() has four additional arguments: the filepath, the buffer
			// to write, the number of bytes to write from the buffer, and the
			// offset from the beginning of the file to write to.
			printf("Got a write request\n");

            char *write_path = strtok(NULL, ",");
            char *buffer = strtok(NULL, ",");
            char *size_str = strtok(NULL, ",");
            char *offset_str = strtok(NULL, ",");

            int size = atoi(size_str);
            int offset = atoi(offset_str);

            int fd = open(write_path, O_RDWR);
            if (fd < 0) {
                char *result = (char *) malloc(sizeof(char) * (size + 20));
                memset(result, 0, size + 20);
                snprintf(result, size + 20, "%d", fd);
                write(client_fd, result, size + 20);
            } else {
                int write_result = pwrite(fd, buffer, size, offset);
                char *result = (char *) malloc(sizeof(char) * 20);
                memset(result, 0, 20);
                snprintf(result, 20, "%d", write_result);
                write(client_fd, result, 20);
            }
		} else if (strcmp(op_type, "flush") == 0) {
			// flush() takes one additional argument: the filepath.
			printf("Got a flush request\n");
			char * flush_path = strtok(NULL, ",");
			printf("Path: %s\n", flush_path);

			int return_code = fflush(flush_path);

			char *ret_str = ((char *) malloc(20));
			memset(ret_str, 0, 20);
			snprintf(ret_str, 20, "%d", return_code);
			write(client_fd, ret_str, 20);

		} else if (strcmp(op_type, "release") == 0) {
			// release() takes one additional argument: the filepath.
			printf("Got a release request\n");
			char * release_path = strtok(NULL, ",");
			printf("Path: %s\n", release_path);

			int return_code = remove(release_path);

			char *ret_str = ((char *) malloc(20));
			memset(ret_str, 0, 20);
			snprintf(ret_str, 20, "%d", return_code);
			write(client_fd, ret_str, 20);

		} else if (strcmp(op_type, "create") == 0) {
			// create() takes two additional arguments: the filepath, and the
			// mode to open the file in.
			printf("Got a create request\n");
            char * create_path = strtok(NULL, ",");
            char * create_flags = strtok(NULL, ",");
            printf("Create path: %s\n", create_path);
            printf("Create flags: %s\n", create_flags);

            int flags = atoi(create_flags);

            int fd = open(create_path, create_flags);
            printf("Create result: %d\n", fd);

            char *result = (char *) malloc(sizeof(char) * 30);
            memset(result, 0, 30);
            snprintf(result, 30, "%d", fd);
            write(client_fd, result, 30);


		} else if (strcmp(op_type, "mkdir") == 0) {
			// mkdir() takes two additional arguments: the directory name, and
			// the permissions to open the directory in.
			printf("Got a mkdir request\n");

			char* path = strtok(NULL, ",");
			int dirmode = atoi(strtok(NULL, ","));
			printf("path: %s\ndirmode: %04o\n", path, dirmode);

			int return_code = mkdir(path, dirmode);

			char *ret_str = ((char *) malloc(20));
			memset(ret_str, 0, 20);
			snprintf(ret_str, 20, "%d", return_code);
			write(client_fd, ret_str, 20);

		} else if (strcmp(op_type, "opendir") == 0) {
			// opendir() takes one additional argument: the directory name.
			printf("Got an opendir request\n");
			char * path = strtok(NULL, ",");
			printf("Path: %s\n", path);

			DIR * result = opendir_handler(path);

			char *ret_str = (char *) malloc(sizeof(int) * 10);
			memset(ret_str, 0, 10);

			if (result == NULL) {
				perror("opendir did not open properly\n");
				snprintf(ret_str, 10, "%d", 0);
				write(client_fd, ret_str, 10);
			}
			else {
				snprintf(ret_str, 10, "%d", -1);
				write(client_fd, ret_str, 10);
			}

		} else if (strcmp(op_type, "readdir") == 0) {
			// return is in the form "<num_entries>,<entry1>,<entry2>,...,<entryn>"
			printf("Got a readdir request\n");
			char * path = strtok(NULL, ",");
    			printf("Path: %s\n", path);

    			char * ret_str = (char *) malloc(sizeof(char) * 10);
    			//try to open the file path
    			DIR * dir = opendir_handler(path);

			//write back null if not possible to read because the path is not opened
			if (dir == NULL) {
	    			perror("The directory does not exist or cannot be opened at this time\n");
	    			snprintf(ret_str, 10, "%d", -1);
	    			write(client_fd, ret_str, 10);
	    		}

			int num_entries = readdir_handle_num_entries(path);
			int stream_len = readdir_handle_length(path);
			ret_str = readdir_handle_string(path, stream_len, num_entries);

			write(client_fd, ret_str, 1000);

		} else if (strcmp(op_type, "releasedir") == 0) {
			// releasedir() takes one additional argument: the directory name.
			printf("Got a releasedir request\n");

			char* path = strtok(NULL, ",");
			printf("path: %s\n", path);

			int return_code = rmdir(path);

			char *ret_str = ((char*) malloc(20));
			memset(ret_str, 0, 20);
			snprintf(ret_str, 20, "%d", return_code);
			write(client_fd, ret_str, 20);

		}

		/*
			The command is not recognized. The server is unable to handle it.
			Error code -1 is returned to the server
		*/
		else {
			printf("Unrecognized request\n");
			char *ret_str = ((char*) malloc(20));
			memset(ret_str, 0, 20);
			snprintf(ret_str, 20, "%d", -1);
			write(client_fd, ret_str, 20);
		}

	}
	close(client_fd);
	return NULL;
}

//TODO: add custom port support here.
int main(int argc, char **argv) {
	printf("Initializing serverSNFS...\n");

	//check to see if the correct number of arguments are passed in by the user
	if (argc != 5) {
		perror("Error: The user did not input the correct number of arugments\n");
		exit(1);
	}

	char * directory_path; //contains the file path where the files are stored by the servers

	//parse the arguments and put them in variables
	int i;
	for (i = 0; i < argc; i++) {
		if (strcmp("-port", argv[i]) == 0) {
			i++;
			port = atoi(argv[i]);
		}
		else if (strcmp("-mount", argv[i]) == 0) {
			i++;
			directory_path = argv[i];
		}
	}

	//check to see that the port and mount path are valid
	if (port == 0) {
		perror("The user did not pass in a good port number or a port at all\n");
	}
	else if (directory_path == NULL) {
		perror("No directory path passed in by the user\n");
	}

	int try_to_connect; //indicates whether port the server was binded or not
	int sock_fd; //indicates whether the socket was successfully constructed
	struct sockaddr_in server_addr; //socket address info struct for the server
	struct sockaddr_in client_addr;	//socket address info struct for the client

	//attempts to build the socket
	sock_fd = socket(AF_INET, SOCK_STREAM, 0);

	//checks to see if the socket was initialized successfully
	if (sock_fd < 0) {
		perror("Construction of socket failed");
		return 1;
	}

	//zeros out the address info struct
	memset(&server_addr, 0, sizeof(server_addr));

	//converts the port from an int to a network int
	server_addr.sin_port = htons(port);

	//sets the network address flag to AF_INET
	server_addr.sin_family = AF_INET;

	//sets the network address flag to accept all interfaces
	server_addr.sin_addr.s_addr = INADDR_ANY;

	//attempts to bind the file descriptor to the port
	try_to_connect = bind(sock_fd, (struct sockaddr *) &server_addr,
			sizeof(server_addr));

	//checks to see if the socket was successfully binded to the port
	if (try_to_connect < 0) {
		perror("Could not bind to port");
		return 1;
	}

	//takes the client socket descriptor and listens on the port to accept any incoming messages
	int client_sock_fd;
	listen(sock_fd, 1);
	int clilen = sizeof(client_addr);

	printf("Server initiated successfully! Now accepting clients\n");

    mount_path = (char *) malloc(sizeof(char) * (strlen(directory_path) + 1));
    memset(mount_path, 0, strlen(directory_path) + 1);
    strncpy(mount_path, directory_path, strlen(directory_path));

    printf("Set mount_path to %s\n", mount_path);

	//continues to accept connections for n amount of clients and any amount of tasks per client. Spawns a new worker thread for each task requested by the client.
	while (1) {
		printf("trying to accept\n");
		client_sock_fd = accept(sock_fd, (struct sockaddr *) &client_addr, &clilen);
		printf("accepted\n");
		pthread_t thread_id;
		if (pthread_create(&thread_id, NULL, &client_handler,
					(void*) &client_sock_fd)) {
			perror("Failed to create thread for client request");
		}
	}

	//checks to see if the client was unable to connect
	if (client_sock_fd < 0) {
		perror("Client connection was rejected");
		return 1;
	}

	printf("Not accepting any more stuff\n");




	return 0;
}
