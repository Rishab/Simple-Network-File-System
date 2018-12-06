#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 54321
#define BUF_SIZE 10

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

        /*
         * create
         * opendir
         * readdir
         * releasedir
         * mkdir
         */

        char *size = strtok(buffer, ",");
        char *op_type = strtok(NULL, ",");

        if (strcmp(op_type, "getattr") == 0) {
            // getattr() has one additional argument: the filepath.
            printf("Got a getattr request\n");

        } else if (strcmp(op_type, "truncate") == 0) {
            // truncate() has two additional arguments: the filepath and 
            // the number of bytes to set the file size to.
            printf("Got a truncate request\n");

        } else if (strcmp(op_type, "open") == 0) {
            // open() has three additional arguments: the filepath, the mode
            // we are opening the file in, and the permissions for the file,
            // which we only use if the file has to be created.
            printf("Got an open request\n");

        } else if (strcmp(op_type, "read") == 0) {
            // read() has three additional arguments: the filepath, the number 
            // of bytes to read, and the offset from the beginning of the file
            // where we should start reading from.
            printf("Got a read request\n");

        } else if (strcmp(op_type, "write") == 0) {
            // write() has four additional arguments: the filepath, the buffer 
            // to write, the number of bytes to write from the buffer, and the
            // offset from the beginning of the file to write to.
            printf("Got a write request\n");
        
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
        
        } else if (strcmp(op_type, "readdir") == 0) {
            // Not sure what extra arguments need to get passed in here.
            printf("Got a readdir request\n");
        
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
