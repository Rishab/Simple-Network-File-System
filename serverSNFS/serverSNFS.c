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
/*
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
*/

/*
 * Thread handler for a client's connection. Handles all requests for a client
 * until no more are sent. Pass in a pointer to the socket descriptor (for
 * reading and writing) in as arg.
 */
void *client_handler(void *arg)
{
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
			buffer = strcat_dynamic(buffer, partial_buffer);
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
			printf("Got a getattr request\n");
			char *path = strtok(NULL, ",");
			strcat(mount, path);
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
    		
			struct dirent * dp;
			
			//check to see if we can read the directory. if so tell the client to read the directory on their end
			if ((dp=readdir(dir)) != NULL) {
				printf("The directory was read successfully\n");
				snprintf(ret_str, 10, "%d", 0);
				write(client_fd, ret_str, 10);
			}
			//if we cannot read the directory, then tell the client not to read the directory on their end
			else {
				perror("The directory was unable to be read\n");
    			snprintf(ret_str, 10, "%d", -1);
    			write(client_fd, ret_str, 10);
			}
    		
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
	
	//continues to accept connections for n amount of clients and any amount of tasks per client. Spawns a new worker thread for each task requested by the client.
	while (client_sock_fd = accept(sock_fd, (struct sockaddr *) &client_addr,
				&clilen)) {
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


	return 0;
}
