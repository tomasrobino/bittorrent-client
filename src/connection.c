#include "connection.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "structs.h"
#include "whole_bencode.h"

address_t* parse_address(const char* address) {
    address_t* ret_address = malloc(sizeof(address_t));

    if (address[0] == 'u') {
        ret_address->protocol = UDP;
    } else if (address[4] == 's') {
        ret_address->protocol = HTTPS;
    } else {
        ret_address->protocol = HTTP;
    }
    ret_address->host = strchr(address, '/') + 2;
    ret_address->port = decode_bencode_int(strchr(ret_address->host, ':')+1, nullptr);
    return ret_address;
}

void send_data_udp(const unsigned short port, const char* ip, const char* data) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        // Error
        //TODO should retry connection or return instead of exiting
        fprintf(stderr, "Socket creation failed");
        exit(2);
    }

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);
}
