#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "../spec.pb-c.h"
#include <arpa/inet.h>
#include "query_constructor.h"

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

        query_t * q = construct(msg);
//        query_t *q2 = malloc(sizeof(query_t));
//        q2->filename = "test.db";
//        q2->target = Q_INDEX;
//        q2->q_type = ADD;
//        attr_type_t * attrs = malloc(2*sizeof (attr_type_t));
//        attrs[0] = (attr_type_t){.type_name = "int_attr", .type = INT};
//        attrs[1] = (attr_type_t){.type_name = "b_attr", .type = BOOL};
//        q2->index = create_index("some_name",attrs, 2);
//        printf("%d\n", q2->index);
//        for (uint32_t i = 0; i < 16; ++i){
//            printf("%x %x\n",(q->filename)[i], (q2->filename)[i]);
//        }
//        printf("%d %d\n", q->q_type, q2->q_type);
//        printf("%d %d\n", q->target, q2->target);
//        index_t * i1 = q->index;
//        index_t * i2 = q2->index;
//        printf("\n\n%d %d\n", i1->count, i2->count);
//        printf("%d %d\n", i1->first_page_num, i2->first_page_num);
//        element_type_t t1 = i1->type;
//        element_type_t t2 = i2->type;
//        printf("\n\n");
//
//        printf("%d %d\n", t1.size, t2.size);
//        for (uint32_t i = 0; i < 16; ++i){
//            printf("%x %x\n",t1.type_name[i], t2.type_name[i]);
//        }
//        printf("\n\n");
//        printf("%d %d\n", t1.kind, t2.kind);
        exec(q);
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
