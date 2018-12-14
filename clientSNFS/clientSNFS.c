#define FUSE_USE_VERSION 31

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fuse.h>
#include <netdb.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>

//#define PORT 54321

//int server_fd = 0;
//int port = 0;

static int fasten() {
	printf("Attempting to fasten to the server\n");
	int port = 16275;
	const char * hostname = "rm.cs.rutgers.edu";
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
		return -1;
	}


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
		return -1;
	}
	return sock;

}

static int snfs_getattr(const char *path, struct stat *fstats,
		struct fuse_file_info *fi) {

	int server_fd = fasten();

	if (server_fd == -1) {
		perror("Failed to connect");
		return -1;
	}

	printf("In the getattr function, path: %s\n", path);

	int size = strlen(path) + 30;
	char *message = (char *) malloc(sizeof(char) * (strlen(path) + 30));
	memset(message, 0, strlen(path) + 30);
	snprintf(message, size, "%d,getattr,%s", strlen(path)+30, path);
	write(server_fd, message, size);
	printf("Sent message to server %s\n", message);

  int result_size = 300;
	char *result = (char *) malloc(result_size);
	memset(result, 0, result_size);

  read(server_fd, result, result_size);
  printf("Getattr result string: %s\n", result);

	int return_code = atoi(strtok(result, ","));
	errno = atoi(strtok(NULL, ","));
	fstats->st_dev = atoi(strtok(NULL, ","));
	fstats->st_ino = atoi(strtok(NULL, ","));
	fstats->st_mode = atoi(strtok(NULL, ","));
	fstats->st_nlink = atoi(strtok(NULL, ","));
	fstats->st_uid = atoi(strtok(NULL, ","));
	fstats->st_gid = atoi(strtok(NULL, ","));
	fstats->st_rdev = atoi(strtok(NULL, ","));
	fstats->st_size = atoi(strtok(NULL, ","));
	fstats->st_atime = atoi(strtok(NULL, ","));
	fstats->st_mtime = atoi(strtok(NULL, ","));
	fstats->st_ctime = atoi(strtok(NULL, ","));
	fstats->st_blksize = atoi(strtok(NULL, ","));
	fstats->st_blocks = atoi(strtok(NULL, ","));

	close(server_fd);

	if (return_code == -1) {
		return -errno;
	}

	return 0;
}


/* truncate
 * Request form: <msg_len>,truncate,0,<path>,<length>
 *           OR  <msg_len>,truncate,1,<fd>,<length>
 */
static int snfs_truncate(const char * path, off_t length,
		struct fuse_file_info *fi) {
	printf("Truncate function called by user\n");
	
    int server_fd = fasten();
    if (server_fd == -1) {
        return -1;
    }

    if (fi == NULL) {
        // Use the file path parameter, since no file info was passed in.
        int size = strlen(path) + 40;
	    char*  message = (char *) malloc(sizeof(char) * size);
    	memset(message, 0, size);
	    snprintf(message, size, "%d,truncate,0,%s,%d", size, path, length);
    	write(server_fd, message, size);    
    } else {
        // Use the file handle in fi instead of the path.
        int size = 50;
        char *message = (char *) malloc(sizeof(char) * size);
        memset(message, 0, size);
        snprintf(message, size, "%d,truncate,1,%d,%d", size, fi->fh, length);
        write(server_fd, message, size);
    }
    
	char *result = (char *) malloc(sizeof(char) * 30);
	memset(result, 0, 30);
	read(server_fd, result, 30);
	printf("%s\n", result);

    int result_code = atoi(strtok(result, ","));
	
    printf("%d\n", result_code);
    
    close(server_fd);
	
    if (result_code == -1) {
        errno = atoi(strtok(result, ","));
        return -errno;
    }

	return 0;
}


/* open
 * Request form: <msg_len>,open,<path>,<flags>
 * Response form: <return_code>,<errno>
 */
static int snfs_open(const char * path, struct fuse_file_info * fi) {
    printf("Open function called by user\n");

    int server_fd = fasten(); 

    int size = strlen(path) + 40;
    char *message = (char *) malloc(sizeof(char) * size);
    memset(message, 0, size);
    snprintf(message, size, "%d,open,%s,%d", size, path, fi->flags);
    write(server_fd, message, size);

    char *result = (char *) malloc(sizeof(char) * 30);
    memset(result, 0, 30);
    read(server_fd, result, 30);
    printf("Open result: %s\n", result);
    
    close(server_fd);

    int return_code = atoi(strtok(result, ","));
    
    if (return_code == -1) {
        errno = atoi(strtok(NULL, ","));
        return -errno;
    }

    fi->fh = return_code;
	return 0;
}


/* read
 * Request form: <msg_len>,read,1,<path>,<size>,<offset>
 *          OR:  <msg_len>,read,0,<fd>,<size>,<offset>
 * Response form: <return_code>,<errno>,<read_buffer>
 */
static int snfs_read(const char * path, char * buffer, size_t size, off_t offset, struct fuse_file_info * fi)
{
    printf("Read function called by user\n");
    
    int server_fd = fasten();

    if (server_fd == -1) {
        return -1;
    }

    if (fi == NULL) {
        // No file info was passed in, so use the path argument to read.
        int msg_size = strlen(path) + 60;
        char *message = (char *) malloc(sizeof(char) * msg_size);
        memset(message, 0, msg_size);
        snprintf(message, msg_size, "%d,read,1,%s,%d,%d", msg_size, path, size, offset);
        write(server_fd, message, msg_size);
    } else {
        // File info was passed in, so use the file handle from fi to read.
        int msg_size = 70;
        char *message = (char *) malloc(sizeof(char) * msg_size);
        memset(message, 0, msg_size);
        snprintf(message, msg_size, "%d,read,0,%d,%d,%d", msg_size, fi->fh, size, offset);
        write(server_fd, message, msg_size);
    }
    
    int res_size = size + 30;
    char *result = (char *) malloc(sizeof(char) * res_size);
    memset(result, 0, size + 20);
    read(server_fd, result, res_size);
    
    printf("Read result: %s\n", result);

    close(server_fd);      // Done reading from the server.
    
    // Need to use strtok_r later to get the read buffer. So we have to
    // allocate a temporary string to use with strtok_r when we get the
    // return code, even though it's essentially useless.
    char *rest = (char *) malloc(sizeof(char) * res_size);
    memset(rest, 0, res_size);

    int result_code = atoi(strtok_r(result, ",", &rest));
    
    printf("Read %d bytes\n", result_code);

    if (result_code == -1) {
        errno = atoi(strtok_r(rest, ",", &rest));
        return -errno;
    } else {
        // result_code holds the number of bytes that were read.
        char *read_string = (char *) malloc(sizeof(char) * (result_code + 1));
        memset(read_string, 0, result_code + 1);
        char *error = strtok_r(rest, ",", &read_string);
        printf("Read string: %s\n", read_string);
        // read_string now holds the buffer that was read from the file.
        strncpy(buffer, read_string, result_code);
        return result_code;
    }
}

/* write
 * Request form: <msg_len>,write,1,<path>,<size>,<offset>,<buffer>
 *          OR:  <msg_len>write,0,<fd>,<size>,<offset>,<buffer>
 * Response form: <response_code>,<errno>
 */
static int snfs_write(const char * path, const char * buffer, size_t size, off_t offset, struct fuse_file_info * fi)
{
    printf("Write function called by user\n");

    int server_fd = fasten();

    if (server_fd == -1) {
        return -1;
    }

    // Copy size bytes from buffer into a null terminated array.
    char *padded_buffer = (char *) malloc(sizeof(char) * (size + 1));
    memset(padded_buffer, 0, size + 1);
    strncpy(padded_buffer, buffer, size);
    
    if (fi == NULL) {
        // No file info, so we will have to open up the path and write.
        int msg_size = strlen(path) + size + 50;
        char *message = (char *) malloc(sizeof(char) * msg_size);    
        memset(message, 0, msg_size);
        snprintf(message, msg_size, "%d,write,1,%s,%d,%d,%s", msg_size, path, size, offset, padded_buffer);
        printf("Command is: %s\n", message);
        write(server_fd, message, msg_size);
    } else {
        // File info was passed in, so we can use the handle from fi.
        int msg_size = size + 60;
        char *message = (char *) malloc(sizeof(char) * msg_size);
        memset(message, 0, msg_size);
        snprintf(message, msg_size, "%d,write,0,%d,%d,%d,%s", msg_size, fi->fh, size, offset, padded_buffer);
        printf("Command is: %s\n", message);
        write(server_fd, message, msg_size);
    }
    
    int result_size = 30;
    char *result = (char *) malloc(sizeof(char) * result_size);
    memset(result, 0, result_size);
    read(server_fd, result, result_size);
    
    printf("Result: %s\n", result);

    close(server_fd);           // Done reading from the server.

    int result_code = atoi(strtok(result, ","));

    if (result_code < 0) {
        errno = atoi(strtok(NULL, ","));
        return -errno;
    }

    return result_code;
}



static int snfs_flush(const char * path, struct fuse_file_info * fi) {
	//called on each close; write back data and return errors
	printf("Making a flush request\n");
	
	//connect the command to the server
	int server_fd = fasten();
	
	int size = strlen(path) + 30;
	char * message = (char *) malloc(sizeof(char) * size);
	memset(message, 0, size);
	snprintf(message, size, "%d,flush,%s", size, path);
	write(server_fd, message, size);
	
	read(server_fd, message, size);
	
	return 0;
}

static int snfs_release(const char * path, struct fuse_file_info * fi) {
	printf("Making a release request\n");
	
	//connect the command to the server
    int server_fd = fasten(); 
    
    //check to see if the file has already been released
    if (fi == NULL) {
    	printf("File is already released\n");
    	return 0;
    }
	
	//set up the message to send to the server that includes the file descriptor
    int size = 30;
    char *message = (char *) malloc(sizeof(char) * size);
    memset(message, 0, size);
    snprintf(message, size, "%d,release,%d", size, fi->fh);
    write(server_fd, message, size);
    
	//check to see if the file closed successfully
    char *result = (char *) malloc(sizeof(char) * 30);
    memset(result, 0, 30);
    read(server_fd, result, 30);
    printf("Release result: %s\n", result);

    int return_code = atoi(strtok(result, ","));
    
    if (return_code == -1) {
        errno = atoi(strtok(NULL, ","));
        return -errno;
    }

	return 0;
}



/* create
 * Request form: <msg_len>,create,<path>,<flags>,<filemodes>
 * Response form: <return_code>,<errno>
 */
static int snfs_create(const char * path, mode_t filemodes, struct fuse_file_info * fi) {
    printf("Client called create\n");

    int server_fd = fasten();

    if (server_fd == -1) {
        return -1;
    }

    int msg_len = strlen(path) + 50;
    char *message = (char *) malloc(sizeof(char) * msg_len);
    memset(message, 0, msg_len);
    snprintf(message, msg_len, "%d,create,%s,%d,%d", msg_len, path, fi->flags, filemodes);
    write(server_fd, message, msg_len);


    int result_size = 30;
    char *result = (char *) malloc(sizeof(char) * result_size);
    memset(result, 0, result_size);

    read(server_fd, result, result_size);

    close(server_fd);       // Done reading responses from the server.

    int result_code = atoi(strtok(result, ","));
    
    if (result_code == -1) {
        errno = atoi(strtok(NULL, ","));
        return -errno;
    }
    
    fi->fh = result_code;
    
	return 0;
}


static int snfs_mkdir(const char * path, mode_t dirmode) {
	int server_fd = fasten();
	int msg_size = strlen(path) + sizeof(mode_t) + 60;
    char *message = (char *) malloc(sizeof(char) * msg_size);
    memset(message, 0, msg_size);
	snprintf(message, msg_size, "%d,mkdir,%s,%d", msg_size, path, (int) dirmode);
	printf("Writing: %s to the server\n", message);
    write(server_fd, message, msg_size);

    char *result = (char *) malloc(sizeof(char) * 30);
    memset(result, 0, 30);
    read(server_fd, result, 30);
    printf("String returned from the server: %s\n", result);

	int result_code = atoi(strtok(result, ","));
    if (result_code == -1) {
        errno = atoi(strtok(NULL, ","));
        printf("The value of errno is: %d\n", errno);
        return -errno;
    }

	return 0;
}


static int snfs_opendir(const char * path, struct fuse_file_info * fi) {
	//check if opendir is permitted?
	//should return the DIR pointer to fuse_file_info?
	printf("Making an opendir request\n");
	/*int size = strlen(path) + 30;
	char *message = (char *) malloc(sizeof(char) * (strlen(path) + 30));
	memset(message, 0, strlen(path) + 30);
	snprintf(message, size, "%d,opendir,%s", strlen(path)+30, path);
	write(server_fd, message, size);

	char *result = (char *) malloc(sizeof(char) * 100);
	memset(result, 0, 100);
	read(server_fd, result, 100);
	printf("%d\n");*/
	return 0;
}


static int snfs_readdir(const char * path, void * buffer,
	fuse_fill_dir_t filler, off_t offset,
	struct fuse_file_info * fi) {

	printf("Client is making a call to readdir\n");

	int server_fd = fasten();
	int size = strlen(path) + 30;
	char *message = (char *) malloc(sizeof(char) * (strlen(path) + 30));
	char * num_bytes_to_read = (char *) malloc(sizeof(char) * 10);
	char * num_entries_str = (char *) malloc(sizeof(char) * 10);
	
	memset(message, 0, strlen(path) + 30);
	snprintf(message, size, "%d,readdir,%s", strlen(path)+30, path);
	write(server_fd, message, size);

	//the client will read the number of bytes needed to read the stream of file names
	memset(num_bytes_to_read, 0, 10);
	read(server_fd, num_bytes_to_read, 10);
	printf("The client will read %s of bytes of filenames\n", num_bytes_to_read);
	
	//the client will read the number of files in the stream of file name
	memset(num_entries_str, 0, 10);
	read(server_fd, num_entries_str, 10);
	printf("The client will read %s entries\n", num_entries_str);

	//convert the client files and names to ints
	int stream_len = atoi(num_bytes_to_read);
	int num_entries = atoi(num_entries_str);
	printf("The client needs to read in a buffer of: %d contianed in %d files\n", stream_len, num_entries);
	
	//prepare to read the from the server the buffer
	int result_len = stream_len + num_entries;
	char * result = (char*) malloc(result_len * sizeof(char));
	memset(result, 0, result_len);
	int bytes_read = read(server_fd, result, result_len);
	printf("Read from the server: %s and read this many bytes %d\n", result, bytes_read);
	
	//tokenize all of the file names and have FUSE use filler to print them out to the terminal
	char * file_name = strtok(result, ",");
	printf("Filename: %s\n", file_name);
	filler(buffer, file_name, NULL, 0);
	int i;
	for (i = 0; i < num_entries; i++) {
		file_name = strtok(NULL, ",");
		printf("Filename: %s\n", file_name);
		if (file_name == NULL) {
			break;
		}
		filler(buffer, file_name, NULL, 0);
	}

	return 0;
}



static int snfs_releasedir(const char * path, struct fuse_file_info * fi) {
	printf("releasedir called\n");
	/*int server_fd = fasten();
	int msg_size = strlen(path) + sizeof(mode_t) + 60;
    char *message = (char *) malloc(sizeof(char) * msg_size);
    memset(message, 0, msg_size);
	snprintf(message, msg_size, "%d,releasedir,%s", msg_size, path);
	printf("Writing: %s to the server\n", message);
    write(server_fd, message, msg_size);

    char *result = (char *) malloc(sizeof(char) * 30);
    memset(result, 0, 30);
    read(server_fd, result, 30);
    printf("String returned from the server: %s\n", result);

	int result_code = atoi(strtok(result, ","));
    if (result_code == -1) {
        errno = atoi(strtok(NULL, ","));
        printf("The value of errno is: %d\n", errno);
        return -errno;
    }*/
	return 0;
}

static struct fuse_operations operations = {
    .getattr  	= snfs_getattr,
    .readdir  	= snfs_readdir,
    .truncate 	= snfs_truncate,
    .open     	= snfs_open,
    .read     	= snfs_read,
    .create   	= snfs_create,
    .write    	= snfs_write,
    .mkdir	  	= snfs_mkdir,
    .releasedir = snfs_releasedir,
    .release	= snfs_release,
    .flush		= snfs_flush,
    .opendir	= snfs_opendir
};

int main(int argc, char **argv)
{
/*
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
	//printf("The directory passed in by the user is: %s\n", directory_path);
*/
/*
	port = 16268;
	const char * hostname = "pwd.cs.rutgers.edu";
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
*/

	struct fuse_args args = FUSE_ARGS_INIT(0, NULL);

	fuse_opt_parse(&args, NULL, NULL, NULL);
	fuse_opt_add_arg(&args, "/freespace/local/mk_serv/");

	printf("Successfully connected to the server\n");

	return fuse_main(argc, argv, &operations, NULL);
}
