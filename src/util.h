#ifndef STRUCTS_H
#define STRUCTS_H

#define CLIENT_ID "-IT0001-"

typedef struct ll {
    struct ll *next;
    char *val;
} ll;

typedef struct {
    struct sockaddr *server_addr;
    int sockfd;
    const char* info_hash;
    const char* peer_id;
} handshake_args_t;

#endif //STRUCTS_H
