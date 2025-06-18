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
    const char* start = strchr(address, '/') + 2;
    const char* end = strchr(start, ':');
    ret_address->host = malloc(sizeof(char)*(end-start+1));
    strncpy(ret_address->host, start, end-start);
    ret_address->host[end-start] = '\0';
    ret_address->port = decode_bencode_int(end+1, nullptr);
    return ret_address;
}

connect_response_t* connect_udp(struct sockaddr server_addr, int sockfd, connect_request_t req) {
    if (req.protocol_id != 0x41727101980) {
        // error
        fprintf(stderr, "Invalid connect request protocol");
        exit(1);
    }

    if (req.action != 0) {
        // error
        fprintf(stderr, "Invalid connect request action");
        exit(1);
    }

    ssize_t sent = sendto(sockfd, &req, sizeof(connect_request_t), 0, &server_addr, sizeof(server_addr));

    if (sent < 0) {
        // error
        fprintf(stderr, "Can't send connect request");
        exit(1);
    }
    fprintf(stdout, "Sent %zd bytes\n", sent);
    


    connect_response_t* res = malloc(sizeof(connect_response_t));
    return res;
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
