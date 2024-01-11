#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "spec.pb-c.h"

#define MAX 80
#define PORT 3110
#define SA struct sockaddr

int main() {
    int sockfd, connfd, len;
    struct sockaddr_in servaddr, cli;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        exit(0);
    }
    bzero(&servaddr, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    if ((bind(sockfd, (SA *) &servaddr, sizeof(servaddr))) != 0) {
        exit(0);
    }

    if ((listen(sockfd, 5)) != 0) {
        exit(0);
    }
    len = sizeof(cli);

    connfd = accept(sockfd, (SA *) &cli, &len);
    if (connfd < 0) {
        exit(0);
    }

    uint8_t buffer[1024];
    uint64_t bytes_received = recv(connfd, &buffer, sizeof(buffer), 0);

    Ast *msg = ast__unpack(NULL, bytes_received, buffer);

    if (msg == NULL) {
        close(connfd);
        exit(EXIT_FAILURE);
    }


    printf("Received Message:\n");
    printf("Type: %s\n", msg->left);
    printf("Value: %s\n", msg->right);
    printf("Value: %s\n", msg->type);

    ast__free_unpacked(msg, NULL);


    close(connfd);
    close(sockfd);
    exit(0);
}