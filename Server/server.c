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

int main()
{
    int sockfd, connfd, len;
    struct sockaddr_in servaddr, cli;

    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    // Binding newly created socket to given IP and verification
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
        printf("socket bind failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully binded..\n");

    // Now server is ready to listen and verification
    if ((listen(sockfd, 5)) != 0) {
        printf("Listen failed...\n");
        exit(0);
    }
    else
        printf("Server listening..\n");
    len = sizeof(cli);

    // Accept the data packet from client and verification
    connfd = accept(sockfd, (SA*)&cli, &len);
    if (connfd < 0) {
        printf("server accept failed...\n");
        exit(0);
    }
    else
        printf("server accept the client...\n");



    // Receive data from the client

    for (;;){

        uint8_t buffer[1024];
        uint64_t bytes_received = recv(connfd, &buffer, sizeof(buffer), 0);



        printf("Bytes Received : %llu\n", bytes_received);
        Ast* msg = ast__unpack(NULL, bytes_received, buffer);

        if (msg == NULL) {
            fprintf(stderr, "Error unpacking the message\n");
            close(connfd);
            exit(EXIT_FAILURE);
        }


        printf("Received Message:\n");
        printf("Type: %s\n", msg->left);
        printf("Value: %s\n", msg->right);
        printf("Value: %s\n", msg->type);

        // Clean up
        ast__free_unpacked(msg, NULL);
        // bytes_received = recv(connfd, &buffer, sizeof(buffer), 0);
    }

    // After chatting close the socket
    close(connfd);
    close(sockfd);
    exit(0);
}