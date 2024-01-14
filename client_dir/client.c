#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/socket.h>
#include "converter.h"
#include "printer.h"
#include "../spec.pb-c.h"

#define PORT 3110
#define SA struct sockaddr

extern int yyparse();

extern ast_node *ast;

void send_message(Ast *msg, int sockfd) {
    uint64_t message_size = ast__get_packed_size(msg);
    uint8_t *buffer = malloc(message_size);
    ast__pack(msg, buffer);
    send(sockfd, buffer, message_size, 0);
    free(buffer);
}

int main() {
    int sockfd;
    struct sockaddr_in servaddr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        exit(0);
    }
    bzero(&servaddr, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(PORT);

    if (connect(sockfd, (SA *) &servaddr, sizeof(servaddr)) != 0) {
        exit(0);
    }


    Ast *msg;
    uint8_t buf[1024];

    do {
        yyparse();
        msg = convert(ast);
        send_message(msg, sockfd);

        uint64_t recieved = recv(sockfd, &buf, 1024, 0);
        Response * res = response__unpack(NULL, recieved, buf);
        printf("%s", res->res);
        response__free_unpacked(res, NULL);
    } while (true);

    return 0;
}
