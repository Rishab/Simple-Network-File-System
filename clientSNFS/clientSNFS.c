#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 54321

int server_fd = 0;

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


int main(int argc, char **argv)
{
    struct sockaddr_in address;
    int sock = 0;
    int read_ret;
    struct sockaddr_in serv_addr;


    //char *hello = "6,open";
    char buffer[1024] = {0};

    int tmp_ret;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket failed");
        return 1;
    }

    server_fd = sock;

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    tmp_ret = connect(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
    if (tmp_ret != 0) {
        perror("connect failed");
        return 1;
    }

    //write(sock, hello, strlen(hello));
    printf("Hello message sent\n");
    printf("%s\n", buffer);
    
    snfs_getattr("./mk1483.txt", NULL, NULL);

    return 0;
}
