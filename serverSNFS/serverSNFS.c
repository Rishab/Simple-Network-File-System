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

	int final_stream_len = stream_length + num_files;
	int final_stream_len_size = num_digits(final_stream_len);
	char *final_stream_len_str = (char *) malloc(sizeof(char) * final_stream_len_size);

	char * files = (char *) malloc(sizeof(char) * final_stream_len);

	int i;
	int j = 0;

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

        // Make a copy of the command for potential future use.
        char *command_copy = (char *) malloc(sizeof(char) * num_bytes);
        strncpy(command_copy, buffer, num_bytes);

		// Now that we have the whole command string, we can parse it to find
		// the operation type.
		int cmd_length = atoi(strtok(buffer, ","));

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
			snprintf(ret_str, 300, "%d,%d,%ld,%ld,%d,%ld,%d,%d,%ld,%ld,%ld,%ld,%ld,%ld,%ld", 
                     ret, 
                     errno,
				     server_stats.st_dev, 
                     server_stats.st_ino, 
                     server_stats.st_mode, 
                     server_stats.st_nlink, 
                     server_stats.st_uid,
                     server_stats.st_gid, 
                     server_stats.st_rdev, 
                     server_stats.st_size, 
                     server_stats.st_atime,
                     server_stats.st_mtime, 
                     server_stats.st_ctime, 
                     server_stats.st_blksize, 
                     server_stats.st_blocks
            );

			printf("Result: %s\n", ret_str);

			write(client_fd, ret_str, ret_str_size);
			return NULL;

		} else if (strcmp(op_type, "truncate") == 0) {
            /* truncate
             * Changes size to number of bytes.
             * Request form: 1,<path>,<size>
             *           OR  0,<fd>,<size>
             * Response form: <return_code>,<errno>
             */
            printf("Got a truncate request\n");
            
            int is_file_open = atoi(strtok(NULL, ","));
            int result_code;
            if (is_file_open) {
                int fd = atoi(strtok(NULL, ","));
                int length = atoi(strtok(NULL, ","));
                printf("Truncate file descriptor: %d\n", fd);
			    printf("Truncate Length: %d\n", length);
                result_code = ftruncate(fd, length);
			} else {
                char *path = strtok(NULL, ",");
                char *absolute_path = strcat_dynamic(mount_path, path, 0);
			    int length = atoi(strtok(NULL, ","));
                printf("Truncate File Path: %s\n", absolute_path);
			    printf("Truncate Length: %d\n", length);
                result_code = truncate(absolute_path, length);
            }

            char *result = (char *) malloc(sizeof(char) * 30);
            memset(result, 0, 30);

            if (result_code == 0) {
                snprintf(result, 30, "%d,0", result_code);
            } else {
                snprintf(result, 30, "%d,%d", result_code, errno);
            }

			write(client_fd, result, 30);
            return NULL;
		
        } else if (strcmp(op_type, "open") == 0) {
			/* open
             * Opens up a file descriptor for a file to write to.
             * Request form: <path>,<flags>
             * Response form: <fd>,<errno>  (fd will be -1 on an error).
             */
            
            printf("Got an open request\n");
            char *open_path = strtok(NULL, ",");
            char *absolute_path = strcat_dynamic(mount_path, open_path, 0);
            int flags = atoi(strtok(NULL, ","));
            
            printf("Open path: %s \t Flags: %d\n", absolute_path, flags);

            int fd = open(absolute_path, flags);
            
            printf("Open result: %d\n", fd);
            
            char *result = (char *) malloc(sizeof(char) * 30);
            memset(result, 0, 30);
            
            if (fd == -1) {
                snprintf(result, 30, "%d,%d", fd, errno);
            } else {
                snprintf(result, 30, "%d,0", fd);
            }
            
            write(client_fd, result, 30);
            return NULL;
		
        } else if (strcmp(op_type, "read") == 0) {
			/* read
             * Reads from file into a buffer.
             * Request form: 1,<path>,<size>,<offset>
             *          OR:  0,<fd>,<size>,<offset>
             * Response form: <return_code>,<errno>,<read_buffer>
             */
			printf("Got a read request\n");
            
            int is_file_unopened = atoi(strtok(NULL, ","));

            int fd;

            if (is_file_unopened) {
                // Have to open the file first before we can read from it.
                char *path = strtok(NULL, ",");
                char *absolute_path = strcat_dynamic(mount_path, path, 0);
                printf("Absolute path is: %s\n", absolute_path);
                fd = open(absolute_path, O_RDONLY);
            } else {
                printf("Reading from a file descriptor\n");
                fd = atoi(strtok(NULL, ","));
            }

            int size = atoi(strtok(NULL, ","));
            int offset = atoi(strtok(NULL, ","));
            
            printf("Read size: %d\n", size);
            printf("Read offset: %d\n", offset);

            int result_size = size + 30;

            char *result = (char *) malloc(sizeof(char) * result_size);
            memset(result, 0, result_size);
            
            if (fd < 0) {
                // There was an error opening the file, or we got a bad fd.
                snprintf(result, result_size, "%d,%d", fd, errno);
                write(client_fd, result, result_size);
                return NULL;
            }
            
            printf("File was successfully opened\n");

            char *buffer = (char *) malloc(sizeof(char) * (size + 1));
            memset(buffer, 0, size + 1);

            int read_bytes = pread(fd, buffer, size, offset);
            
            printf("Read bytes: %d\n", read_bytes);

            if (read_bytes == -1) {
                // There was an error reading the file.
                snprintf(result, result_size, "%d,%d", read_bytes, errno);
                write(client_fd, result, result_size);
                return NULL;
            }
            
            if (is_file_unopened) {
                // If the file started closed, make sure it ends closed.
                close(fd);
            }

            snprintf(result, result_size, "%d,0,%s", read_bytes, buffer);
            write(client_fd, result, result_size);
            return NULL;    

        } else if (strcmp(op_type, "write") == 0) {
            // write() has four additional arguments: the filepath, the buffer
			// to write, the number of bytes to write from the buffer, and the
			// offset from the beginning of the file to write to.
			printf("Got a write request\n");
            
            int is_file_unopened = atoi(strtok(NULL, ","));
            int fd;
            
            if (is_file_unopened) {
                // Unknown file descriptor. Need to determine from path.
                char *path = strtok(NULL, ",");
                char *absolute_path = strcat_dynamic(mount_path, path, 0);
                fd = open(absolute_path, O_WRONLY);
            } else {
                // Working with already known file descriptor.
                fd = atoi(strtok(NULL, ","));
            }
            
            int result_size = 30;
            printf("fd: %d\n", fd);
            if (fd == -1) {
                char *result = (char *) malloc(sizeof(char) * result_size);
                memset(result, 0, result_size);
                snprintf(result, result_size, "%d,%d", fd, errno);
                write(client_fd, result, result_size);
                return NULL;
            }

            printf("command copy: %s\n", command_copy);
            char *rest = (char *) malloc(sizeof(char) * num_bytes);
            
            // Go through all the arguments that we know so we can get to
            // the size.
            strtok_r(command_copy, ",", &rest);     // Get command length
            strtok_r(rest, ",", &rest);             // Get command type
            strtok_r(rest, ",", &rest);             // Get opened or unopened file status
            strtok_r(rest, ",", &rest);             // Get fd/path

            int size = atoi(strtok_r(rest, ",", &rest));
            int offset = atoi(strtok_r(rest, ",", &rest));
            char *write_buf = rest;

            printf("Write buffer: %s\n", write_buf);

            int write_bytes = pwrite(fd, write_buf, size, offset);
            
            if (write_bytes < 0) {
                char *result = (char *) malloc(sizeof(char) * result_size);
                memset(result, 0, result_size);
                snprintf(result, result_size, "%d,%d", write_bytes, errno);
                write(client_fd, result, result_size);
                if (is_file_unopened) {
                    close(fd);
                }
                return NULL;
            }

            char *result = (char *) malloc(sizeof(char) * result_size);
            memset(result, 0, result_size);
            snprintf(result, result_size, "%d,0", write_bytes);
            write(client_fd, result, result_size);
            if (is_file_unopened) {
                close(fd);
            }

            return NULL;

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
            /* create
             * Like open, but it creates a file if it doesn't exist.
             * Request form: <path>,<flags>,<filemode>
             * Response form: <return_code>,<errno>
             */
			printf("Got a create request\n");

            char *create_path = strtok(NULL, ",");
            char *absolute_path = strcat_dynamic(mount_path, create_path, 0);
            int flags = atoi(strtok(NULL, ","));
            int filemode = atoi(strtok(NULL, ","));
            
            printf("Create path: %s\n", create_path);
            printf("Create flags: %d\n", flags);
            printf("File mode: %d\n", filemode);


            int fd = open(absolute_path, flags, filemode);
            printf("Create result: %d\n", fd);

            char *result = (char *) malloc(sizeof(char) * 30);
            memset(result, 0, 30);

            if (fd == -1) {
                snprintf(result, 30, "%d,%d", fd, errno);
            } else {
                snprintf(result, 30, "%d,0", fd);
            }

            write(client_fd, result, 30);
            return NULL;

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
			printf("The server received a readdir request\n");
			char * path = strtok(NULL, ",");
    		printf("Path: %s\n", path);
            char *absolute_path = strcat_dynamic(mount_path, path, 0);
    		char * num_bytes_to_write = (char *) malloc(sizeof(char) * 10);
    		char * num_entries_str = (char *) malloc(sizeof(char) * 10);
    		char * ret_str;
    		
    		//try to open the file path
    		DIR * dir = opendir_handler(absolute_path);

			//write back null if not possible to read because the path is not opened
			if (dir == NULL) {
	    			perror("The directory does not exist or cannot be opened at this time\n");
	    			snprintf(ret_str, 10, "%d", -1);
	    			write(client_fd, ret_str, 10);
	    	}
	    	
			int num_entries = readdir_handle_num_entries(absolute_path);
			int stream_len = readdir_handle_length(absolute_path);
			
			memset(num_bytes_to_write, 0, 10);
			snprintf(num_bytes_to_write, 10, "%d", stream_len);
			printf("The client will read %s bytes of filenames\n", num_bytes_to_write);
			write(client_fd, num_bytes_to_write, 10);
			
			memset(num_entries_str, 0, 10);
			snprintf(num_entries_str, 10, "%d", num_entries);
			printf("The client will read %s bytes of entries\n", num_entries_str);
			write(client_fd, num_entries_str, 10);
				
			ret_str = readdir_handle_string(absolute_path, stream_len, num_entries);
			printf("Writing the readdir request to the client: %s\n", ret_str);
			write(client_fd, ret_str, num_entries+stream_len);

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
