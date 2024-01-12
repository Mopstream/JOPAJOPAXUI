#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "../spec.pb-c.h"
#include <arpa/inet.h>

#define PORT 3110
#define SA struct sockaddr


void server_jobs(int connfd, struct sockaddr_in cli) {

    for (;;) {
        uint8_t buffer[1024];
        uint64_t bytes_received = recv(connfd, &buffer, 1024, 0);
        if (!bytes_received) {
            printf("Disconnected!\n");
            return;
        }

        Ast *msg = ast__unpack(NULL, bytes_received, buffer);

        if (msg == NULL) {
            fprintf(stderr, "Error unpacking the message\n");
            close(connfd);
            exit(EXIT_FAILURE);
        }


        printf("Received Message: ");
        printf("%zu\n", msg);
        int flag = 1;
        send(connfd, &flag, sizeof(int), 0);

        ast__free_unpacked(msg, NULL);
    }

    close(connfd);
    exit(0);
}

int main() {
    int sockfd, connfd;
    uint32_t len;
    struct sockaddr_in servaddr, cli;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    }
    bzero(&servaddr, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    if ((bind(sockfd, (SA *) &servaddr, sizeof(servaddr))) != 0) {
        printf("socket bind failed...\n");
        exit(0);
    }

    if ((listen(sockfd, 5)) != 0) {
        printf("Listen failed...\n");
        exit(0);
    }
    len = sizeof(cli);


    listen_loop:;
    connfd = accept(sockfd, (SA *) &cli, &len);
    if (connfd < 0) {
        printf("server accept failed...\n");
        exit(0);
    } else if (!fork()) {
        printf("Got Client connection: %d %s\n", connfd, inet_ntoa(cli.sin_addr));
        server_jobs(connfd, cli);
    } else {
        goto listen_loop;
    }


}
