#include "connection.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "structs.h"
#include "whole_bencode.h"

address_t* split_address(const char* address) {
    address_t* ret_address = malloc(sizeof(address_t));
    memset(ret_address, 0, sizeof(address_t));

    if (strncmp(address, "udp", 3) == 0) {
        ret_address->protocol = UDP;
    } else if (strncmp(address, "http", 5) == 0) {
        ret_address->protocol = HTTP;
    } else if (address[4] == 's') {
        ret_address->protocol = HTTPS;
    } else return nullptr;
    const char* start = strchr(address, '/') + 2;
    const char* end = strchr(start, ':');
    if (end) {
        // has port
        ret_address->host = malloc(sizeof(char)*(end-start+1));
        strncpy(ret_address->host, start, end-start);
        ret_address->host[end-start] = '\0';

        start = end+1;
        end = strchr(start, '/');
        int dif;
        if (end == NULL) {
            dif = (int) strlen(start);
        } else {
            dif = (int) (end-start);
        }
        ret_address->port = (char*) start;
        ret_address->port = malloc(sizeof(char)* (dif+1) );
        strncpy(ret_address->port, start, dif);
        ret_address->port[dif] = '\0';
    } else {
        // no port
        ret_address->port = nullptr;
    }
    return ret_address;
}

char* url_to_ip(address_t address) {
    if (address.protocol == UDP) {
        struct addrinfo hints = {0}, *res;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;

        const int err = getaddrinfo(address.host, address.port, &hints, &res);
        if (err != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
            return nullptr;
        }

        // Print IP address
        char* ip = malloc(INET_ADDRSTRLEN);
        const struct in_addr* addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
        inet_ntop(res->ai_family, addr, ip, INET_ADDRSTRLEN);
        printf("Resolved IP: %s\n", ip);
        freeaddrinfo(res);
        return ip;
    } else { // TODO other protocols besides UDP
        return nullptr;
    }
}

connect_response_t* connect_udp(struct sockaddr* server_addr, int sockfd, unsigned int transaction_id) {
    connect_request_t* req = malloc(sizeof(connect_request_t));
    req->protocol_id = 0x41727101980;
    req->action = 0;
    req->transaction_id = transaction_id;

    const ssize_t sent = sendto(sockfd, req, sizeof(connect_request_t), 0, server_addr, sizeof(struct sockaddr));
    free(req);
    if (sent < 0) {
        // error
        fprintf(stderr, "Can't send connect request");
        exit(1);
    }
    fprintf(stdout, "Sent %zd bytes\n", sent);

    connect_response_t* res = malloc(sizeof(connect_response_t)+1);
    char* buffer = (char*) res;
    socklen_t socklen = sizeof(*server_addr);
    ssize_t recv_bytes = recvfrom(sockfd, buffer, sizeof(connect_response_t), 0, server_addr, &socklen);
    if (recv_bytes < 0) {
        fprintf(stderr, "No response");
    } else {
        buffer[sizeof(connect_response_t)] = '\0';
        fprintf("Server response:\n%s", buffer);
    }

    return res;
}

void download(const char* raw_address) {
    address_t* split_addr = split_address(raw_address);
    char* ip = url_to_ip(*split_addr);
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        // Error
        fprintf(stderr, "Socket creation failed");
        exit(2);
    }

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(decode_bencode_int(split_addr->port, nullptr));
    server_addr.sin_addr.s_addr = inet_addr(ip);
    connect_response_t* connect_response = connect_udp((struct sockaddr*) &server_addr, sockfd, arc4random());

    free(connect_response);
    free(split_addr);
}