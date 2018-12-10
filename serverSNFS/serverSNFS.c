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

#define PORT 54321
#define BUF_SIZE 10

/* For Mohit: not the best way of finding the number of files and directories. I will figure
 * out a better way soon
 */
int num_files = 0;
int num_directories = 0;

/*
 * Takes in two malloc'ed strings, and concatenates them str1 | str2.
 * If either str1 or str2 is NULL, it returns the non-NULL one.
 * If both str1 and str2 are NULL, it returns NULL.
 * Also frees the memory for str1 and str2 assuming they are not NULL.
 */
char *strcat_dynamic(char *str1, char *str2)
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
	strcat(concat_str, str2);

	// Free memory for input strings.
	free(str1);
	free(str2);

	return concat_str;
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
	
	num_files = 0;
	int files_length = 0;
	
	while ((dp=readdir(dir)) != NULL) {
		printf("File name: %s\n", dp->d_name);
		files_length += strlen(dp->d_name);
		num_files++;
	}
	
	printf("The length of the file stream is: %d\n", files_length);
    return files_length; 
}

char * readdir_handle_string(char * file_path, int stream_length, int num_files) {

	char * files = (char *) malloc(sizeof(char) * stream_length + num_files);
	DIR * dir =  opendir(file_path);
	struct dirent * dp;
		
	if (dir == NULL) {
		return NULL;
	}
	
	int i = 0;
	
	while ((dp=readdir(dir)) != NULL) {
		printf("File name: %s\n", dp->d_name);
		int j = 0;
		for (j = 0; j < strlen(dp->d_name); j++) {
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
	int client_fd = *((int *) arg);
	char *buffer = (char *) malloc(sizeof(char) * BUF_SIZE);
	char *partial_buffer = NULL;

	int read_len = read(client_fd, buffer, BUF_SIZE);

	int i = 0, j = 0;
	int num_bytes = 0;

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
			buffer = strcat_dynamic(buffer, partial_buffer);
		}
		printf("Command: %s\n", buffer);

		// Now that we have the whole command string, we can parse it to find
		// the operation type.

		char *size = strtok(buffer, ",");
		char *op_type = strtok(NULL, ",");

		if (strcmp(op_type, "getattr") == 0) {
			// getattr() has one additional argument: the filepath.
			printf("Got a getattr request\n");
			char *path = strtok(NULL, ",");
			printf("Path: %s\n", path);
			struct stat stats;
			int ret = stat(path, &stats);

			if (ret != 0) {
				perror("error in stat");
			}

			char *ret_str = (char *) malloc(sizeof(int) * 10);
			memset(ret_str, 0, 10);
			snprintf(ret_str, 10, "%d", ret);
			write(client_fd, ret_str, 10);

		} else if (strcmp(op_type, "truncate") == 0) {
			// truncate() has two additional arguments: the filepath and
			// the number of bytes to set the file size to.
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

		} else if (strcmp(op_type, "open") == 0) {
			// open() has three additional arguments: the filepath, the mode
			// we are opening the file in, and the permissions for the file,
			// which we only use if the file has to be created.
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

		} else if (strcmp(op_type, "read") == 0) {
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

		} else if (strcmp(op_type, "release") == 0) {
			// release() takes one additional argument: the filepath.
			printf("Got a release request\n");

		} else if (strcmp(op_type, "create") == 0) {
			// create() takes two additional arguments: the filepath, and the
			// mode to open the file in.
			printf("Got a create request\n");

		} else if (strcmp(op_type, "mkdir") == 0) {
			// mkdir() takes two additional arguments: the directory name, and
			// the permissions to open the directory in.
			printf("Got a mkdir request\n");

		} else if (strcmp(op_type, "opendir") == 0) {
			// opendir() takes one additional argument: the directory name.
			printf("Got an opendir request\n");
			char * path = strtok(NULL, ",");
			printf("Path: %s\n", path);
					
			DIR * result = opendir_handler(path);
			
			if (result == NULL) {
				perror("opendir did not open properly\n");
			}
			
			/* Sends back a DIR pointer to the client */
			write(client_fd, result, sizeof(result));

		} else if (strcmp(op_type, "readdir") == 0) {
			printf("Got a readdir request\n");	
			char * path = strtok(NULL, ",");
    		printf("Path: %s\n", path);
    		int stream_length = readdir_handle_length(path);
    		printf("Server Readdir: The number of files in the directory are: %d and the length of the stream is: %d\n", num_files, stream_length);
    		
    		int result_str_length = stream_length + num_files;
    		char * files = (char *) malloc(sizeof(char) * result_str_length);
    		files = readdir_handle_string(path, stream_length, num_files);
    		
    		printf("The length of the resulting string is: %d, %s\n", result_str_length, files);
    		char * result_str = (char *) malloc (sizeof(int) * (result_str_length));
			memset(result_str, 0, result_str_length);
			snprintf(result_str, result_str_length + 10, "%d,%s", stream_length, files);
    		write(client_fd, result_str, result_str_length + 10);
    		
    		
		} else if (strcmp(op_type, "releasedir") == 0) {
			// releasedir() takes one additional argument: the directory name.
			printf("Got a releasedir request\n");

		} else {
			// Unrecognized command. Handle the error.
			printf("Unrecognized request\n");
		}

	}

	return NULL;
}

//TODO: add custom port support here.
int main(int argc, char **argv)
{
	printf("Initializing serverSNFS...\n");

	int tmp_ret;  // Holds return values of any operations we do.
	int sock_fd;

	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;

	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_fd < 0) {
		perror("Construction of socket failed");
		return 1;
	}

	memset(&server_addr, 0, sizeof(server_addr));

	server_addr.sin_port = htons(PORT);
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;

	tmp_ret = bind(sock_fd, (struct sockaddr *) &server_addr,
			sizeof(server_addr));
	if (tmp_ret < 0) {
		perror("Could not bind to port");
		return 1;
	}

	int client_sock_fd;

	listen(sock_fd, 1);

	int clilen = sizeof(client_addr);

	while (client_sock_fd = accept(sock_fd, (struct sockaddr *) &client_addr,
				&clilen)) {
		pthread_t thread_id;
		if (pthread_create(&thread_id, NULL, &client_handler,
					(void*) &client_sock_fd)) {
			perror("Failed to create thread for client request");
		}
	}

	if (client_sock_fd < 0) {
		perror("Client connection was rejected");
		return 1;
	}


	return 0;
}
