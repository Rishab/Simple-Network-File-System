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
    }

    printf("read() failed");

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