#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fuse.h>
#include <netdb.h>

#include <sys/socket.h>
#include <netinet/in.h>

//#define PORT 54321

int server_fd = 0;
int port = 0;

static int snfs_getattr(const char *path, struct stat *fstats,
		struct fuse_file_info *fi)
{
	int size = strlen(path) + 30;
	char *message = (char *) malloc(sizeof(char) * (strlen(path) + 30));
	memset(message, 0, strlen(path) + 30);
	snprintf(message, size, "%d,getattr,%s", strlen(path)+30, path);
	write(server_fd, message, size);

	char *result = (char *) malloc(sizeof(char) * 30);
	memset(result, 0, 30);
	read(server_fd, result, 30);
	printf("%s\n", result);

	int server_return = atoi(result);

	printf("%d\n", server_return);
	return 0;
}

static int snfs_truncate(const char * path, off_t length,
		struct fuse_file_info *fi) {
	//TODO: check if file is not opened
	printf("Truncate function called by user\n");
	int size = strlen(path) + 30;
	char*  message = (char *) malloc(sizeof(char) * (strlen(path) + 30));
	memset(message, 0, strlen(path) + 30);
	snprintf(message, size, "%d,truncate,%s,%d", strlen(path) + 30, path, length);
	write(server_fd, message, size);

	char *result = (char *) malloc(sizeof(char) * 30);
	memset(result, 0, 30);
	read(server_fd, result, 30);
	printf("%s\n", result);

	int server_return = atoi(result);

	printf("%d\n", server_return);

	return 0;

}
static int snfs_open(const char * path, struct fuse_file_info * fi) {
    printf("Open function called by user\n");
    int size = strlen(path) + 30;
    char *message = (char *) malloc(sizeof(char) * size);
    memset(message, 0, size);
    snprintf(message, size, "%d,open,%s", size, path);
    write(server_fd, message, size);

    char *result = (char *) malloc(sizeof(char) * 30);
    memset(result, 0, 30);
    read(server_fd, result, 30);
    printf("Open result: %s\n", result);

    int server_return = atoi(result);

    if (server_return < 0) {
        perror("Server error on open\n");
        return server_return;
    }
    
    int fd = open(path, fi->flags);
    
    if (fd < 0) {
        perror("Client error on open\n");
        return fd;
    }
    fi->fh = fd;
	return 0;
}

static int snfs_read(const char * path, char * buffer, size_t size, off_t offset, struct fuse_file_info * fi) 
{
    printf("Read functionn called by user\n");
    int msg_size = strlen(path) + 60;
    char *message = (char *) malloc(sizeof(char) * msg_size);
    memset(message, 0, msg_size);
    snprintf(message, size, "%d,read,%s,%d,%d", msg_size, path, (int) size, (int) offset);
    write(server_fd, message, msg_size);

    char *result = (char *) malloc(sizeof(char) * (size + 20));
    memset(result, 0, size + 20);
    read(server_fd, result, size+20);

    char *result_code_str = strtok(result, ",");
    char *read_buffer = strtok(NULL, ",");

    int result_code = atoi(result_code_str);
    if (result_code < 0) {
        perror("Server error on read\n");
        return result_code;
    }
    
    int fd;

    if (fi == NULL) {
        fd = open(path, fi->flags);
    } else {
        fd = fi->fh;
    }

    int client_result = pread(fd, buffer, size, offset);
    if (client_result < 0) {
        perror("Client error on read\n");
        return client_result;
    }

    if (strcmp(buffer, read_buffer) != 0) {
        printf("WARNING: the read operation succeeded, but there is a consistency error between client and server. Data may not match up on future operations.\n");
    }
    
    if (fi == NULL) {
        close(fd);
    }

	return client_result;
}

static int snfs_write(const char * path, const char * buffer, size_t size, off_t offset, struct fuse_file_info * fi)
{
    int msg_size = strlen(path) + size + 60;
    char *message = (char *) malloc(sizeof(char) * msg_size);
    memset(message, 0, msg_size);
    snprintf(message, msg_size, "%d,write,%s,%s,%d,%d", msg_size, path, buffer, (int) size, (int) offset);
    write(server_fd, message, msg_size);

    char *result = (char *) malloc(sizeof(char) * 20);
    memset(result, 0, 20);
    read(server_fd, result, 20);
    
    int server_return_code = atoi(result);
    if (server_return_code < 0) {
        perror("Server error on write\n");
        return server_return_code;
    }

    int fd;

    if (fi == NULL) {
        fd = open(path, fi->flags);
    } else {
        fd = fi->fh;
    }

    int client_return_code = pwrite(fd, buffer, size, offset);
    
    if (fi == NULL) {
        close(fd);
    }
    
    if (client_return_code != server_return_code) {
        printf("WARNING: Consistency error on write. Data may not be what you expect it to be in the future\n");
    }

    return client_return_code;
} 

static int snfs_flush(const char * path, struct fuse_file_info * fi) {
	return 0;
}

static int snfs_release(const char * path, struct fuse_file_info * fi) {
	return 0;
}

static int snfs_create(const char * path, mode_t filemodes, struct fuse_file_info * fi) {
	return 0;
}

static int snfs_mkdir(const char * path, mode_t dirmode) {
	int msg_size = strlen(path) + sizeof(mode_t) + 60;
    	char *message = (char *) malloc(sizeof(char) * msg_size);
    	memset(message, 0, msg_size);
	snprintf(message, msg_size, "%d,mkdir,%s,%d", msg_size, path, (int) dirmode);
    	write(server_fd, message, msg_size);

    	char *result = (char *) malloc(sizeof(char) * 20);
    	memset(result, 0, 20);
    	read(server_fd, result, 20);

	int server_return_code = atoi(result);
    	if (server_return_code < 0) {
        	perror("Server error on write\n");
        	return server_return_code;
    	}

	//write on server was successful; write on client
	int client_return_code = mkdir(path, dirmode);
	
	if (client_return_code < 0) {
        	printf("WARNING: Consistency error on mkdir. Success on server but failure on client\n");
    	}
	
	return 0;
}

static int snfs_opendir(const char * path, struct fuse_file_info * fi) {
	//check if opendir is permitted?
	//should return the DIR pointer to fuse_file_info?
	int size = strlen(path) + 30;
	char *message = (char *) malloc(sizeof(char) * (strlen(path) + 30));
	memset(message, 0, strlen(path) + 30);
	snprintf(message, size, "%d,opendir,%s", strlen(path)+30, path);
	write(server_fd, message, size);

	char *result = (char *) malloc(sizeof(char) * 100);
	memset(result, 0, 100);
	read(server_fd, result, 100);
	printf("%d\n");
	return 0;
}

static int snfs_readdir(const char * path, void * buffer,
		fuse_fill_dir_t filler, off_t offset,
		struct fuse_file_info * fi) {

	int size = strlen(path) + 30;
	char *message = (char *) malloc(sizeof(char) * (strlen(path) + 30));
	memset(message, 0, strlen(path) + 30);
	snprintf(message, size, "%d,readdir,%s", strlen(path)+30, path);
	write(server_fd, message, size);

	char *result = (char *) malloc(sizeof(char) * 100);
	memset(result, 0, 100);
	read(server_fd, result, 100);
	printf("String returned from the server: %s\n", result);
	printf("Files returned from the server\n");
	
	char * current_token = strtok(result, ",");
	while ((current_token = strtok(NULL, ",")) != NULL) {
		printf("%s\n", current_token);
	}
	return 0;
}

static int snfs_releasedir(const char * path, struct fuse_file_info * fi) {

	int msg_size = strlen(path) + 60;
    	char *message = (char *) malloc(sizeof(char) * msg_size);
    	memset(message, 0, msg_size);
	snprintf(message, msg_size, "%d,releasedir,%s", msg_size, path);
    	write(server_fd, message, msg_size);

    	char *result = (char *) malloc(sizeof(char) * 20);
    	memset(result, 0, 20);
    	read(server_fd, result, 20);

	int server_return_code = atoi(result);
    	if (server_return_code < 0) {
        	perror("Server error on releasedir\n");
        	return server_return_code;
    	}

	//releasedir on server was successful; releasedir (rmdir) on client
	int client_return_code = rmdir(path);
	
	if (client_return_code < 0) {
        	printf("WARNING: Consistency error on releasedir. Success on server but failure on client\n");
    	}
	
	return 0;
}

int main(int argc, char **argv)
{
	//check to see if the correct number of arguments are passed in
	if (argc != 7) {
		perror("Error: The user did not input the correct number of arugments\n");
		exit(1);
	}

	char * directory_path; //contains the file path where the files are stored by the server
	char * hostname; //contains the name of the hostname where serverSNFS is located
	
	//parse the arguments and put them in variables
	int i;
	for (i = 0; i < argc; i++) {
		if (strcmp("-port", argv[i]) == 0) {
			i++;
			port = atoi(argv[i]);
		}
		else if (strcmp("-address", argv[i]) == 0) {
			i++;
			hostname = argv[i];
		}
		else if (strcmp("-mount", argv[i]) == 0) {
			i++;
			directory_path = argv[i];
		}
	}

	//check to see that valid port number was passed in by the user
	if (port < 0) {
		perror("The user did not pass in a proper port\n");
	}
	
	else if (hostname == NULL) {
		perror("Server was not initialized?\n");
	}
	printf("The port passed in by the user is: %d\n", port);
	printf("The hostname passed in by the user is: %s\n", hostname);
	printf("The directory passed in by the user is: %s\n", directory_path);

	struct sockaddr_in address;
	int sock = 0;
	int read_ret;
	struct sockaddr_in serv_addr;
	
	//puts the server's IP into the server_ip struct
	struct hostent * server_ip = gethostbyname(hostname);
	
	char buffer[1024] = {0};

	int try_to_connect;

	//attempts to build a socket as a gateway to connect to the server
	sock = socket(AF_INET, SOCK_STREAM, 0);
	
	//checks to see if the socket was successfully created
	if (sock < 0) {
		perror("socket failed");
		return 1;
	}

	server_fd = sock;
	
	//zeros out the server_addr struct
	memset(&serv_addr, 0, sizeof(serv_addr));
	
	//sets the network address flag to AF_INET
	serv_addr.sin_family = AF_INET;
	
	//converts the port from int to a network int and retrieves the endianness of the machine
	serv_addr.sin_port = htons(port);
	
	//copies the bytes for the server's IP address into the server_addr struct
	bcopy((char *) server_ip->h_addr, (char *) &serv_addr.sin_addr, server_ip->h_length);

	try_to_connect = connect(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
	if (try_to_connect != 0) {
		perror("connect failed");
		return 1; 
	}

	// write(sock, hello, strlen(hello));
	printf("Hello message sent\n");
	printf("%s\n", buffer);

	// snfs_getattr("./mk1483.txt", NULL, NULL);
	//snfs_truncate("./mk1483.txt\0", 10, NULL);

	//snfs_readdir("./tmp", NULL, 0, NULL, NULL);
	snfs_opendir("./tmp", NULL);
	return 0;
}
