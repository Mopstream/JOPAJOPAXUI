#include "ast.h"
#include "printer.h"
#include "parser.tab.h"
#include "converter.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX 80
#define PORT 3110
#define SA struct sockaddr


void send_message(Ast *msg, int sockfd) {
    uint64_t message_size = ast__get_packed_size(msg);
//    printf("Size of message is %llu\n", message_size);
    uint8_t *buffer = malloc(message_size);
    ast__pack(msg, buffer);
    send(sockfd, buffer, message_size, 0);
    free(buffer);
}


extern int yydebug;
extern ast_node *ast;
//extern int yyparse();

int main() {
    if (YYDEBUG == 1) {
        yydebug = 1;
    }
    int sockfd;
    struct sockaddr_in servaddr;

    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        //printf("socket creation failed...\n");
        exit(0);
    } else
        //printf("Socket successfully created..\n");
        bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(PORT);


    // connect the client socket to server socket
    if (connect(sockfd, (SA *) &servaddr, sizeof(servaddr))
        != 0) {
        printf("connection with the server failed...\n");
        exit(0);
    }
//    printf("connected to the server..\n");

    yyparse();
    print_node(ast, 0);

    Ast *msg = convert(ast);
    //send_message(*msg, sockfd);


    return 0;
}
