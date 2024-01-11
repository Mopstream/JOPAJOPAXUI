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
    uint8_t *buffer = malloc(message_size);
    ast__pack(msg, buffer);
    send(sockfd, buffer, message_size, 0);
    free(buffer);
}


extern int yydebug;
extern ast_node *ast;

int main() {
    int sockfd;
    struct sockaddr_in servaddr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        exit(0);
    } else
        bzero(&servaddr, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(PORT);

    if (connect(sockfd, (SA *) &servaddr, sizeof(servaddr)) != 0) {
        exit(0);
    }

    yyparse();
    print_node(ast, 0);

    Ast *msg = convert(ast);
    send_message(msg, sockfd);


    return 0;
}
