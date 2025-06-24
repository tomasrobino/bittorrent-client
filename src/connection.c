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
    } else if (strncmp(address, "http", 4) == 0 && address[4] != 's') {
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
    struct addrinfo hints = {0}, *res;
    hints.ai_family = AF_UNSPEC;
    char* ip = nullptr;
    if (address.protocol == UDP) {
        hints.ai_socktype = SOCK_DGRAM;
    } else hints.ai_socktype = SOCK_STREAM;
    const int err = getaddrinfo(address.host, address.port, &hints, &res);
    if (err != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
        return nullptr;
    }

    // Iterating over received IPs
    for (struct addrinfo *rp = res; rp != nullptr; rp = rp->ai_next) {
        void* addr_ptr;

        address.ip_version = rp->ai_family;
        //IPv6
        if (rp->ai_family == AF_INET6) {
            char buf[INET6_ADDRSTRLEN];
            addr_ptr = &((struct sockaddr_in6 *)rp->ai_addr)->sin6_addr;
            inet_ntop(rp->ai_family, addr_ptr, buf, sizeof(buf));
            ip = malloc(INET6_ADDRSTRLEN);
            if (ip) strcpy(ip, buf);
            printf("Resolved IPv6: %s\n", ip);
            break;
        }

        //IPv4. Doesn't break, trying to get an IPv6
        if (rp->ai_family == AF_INET) {
            char buf[INET_ADDRSTRLEN];
            addr_ptr = &((struct sockaddr_in *)rp->ai_addr)->sin_addr;
            inet_ntop(rp->ai_family, addr_ptr, buf, sizeof(buf));
            ip = malloc(INET_ADDRSTRLEN);
            if (ip) strcpy(ip, buf);
            printf("Resolved IPv4: %s\n", ip);
        }
    }
    freeaddrinfo(res);
    return ip;
}

connect_response_t* connect_udp(struct sockaddr_in* server_addr, int sockfd, unsigned int transaction_id) {
    connect_request_t* req = malloc(sizeof(connect_request_t));
    memset(req, 0, sizeof(connect_request_t));
    // Convert to network endianness
    req->protocol_id = htobe64(0x41727101980LL);
    req->action = htobe32(0);
    req->transaction_id = htobe32(transaction_id);
    fprintf(stdout, "Connection request:\n");
    fprintf(stdout, "action: %d\n", req->action);
    fprintf(stdout, "transaction_id: %d\n", req->transaction_id);
    fprintf(stdout, "protocol_id: %lu\n", req->protocol_id);

    const ssize_t sent = sendto(sockfd, req, sizeof(connect_request_t), 0, (struct sockaddr*) server_addr, sizeof(struct sockaddr_in));
    if (sent < 0) {
        // error
        fprintf(stderr, "Can't send connect request");
        exit(1);
    }
    fprintf(stdout, "Sent %zd bytes\n", sent);

    connect_response_t* res = malloc(sizeof(connect_response_t));
    socklen_t socklen = sizeof(struct sockaddr_in);
    ssize_t recv_bytes = recvfrom(sockfd, res, sizeof(connect_response_t), 0, nullptr, &socklen);
    if (recv_bytes < 0) {
        fprintf(stderr, "No response");
    } else {
        fprintf(stdout, "Server response:\n");
        fprintf(stdout, "action: %d\n", res->action);
        fprintf(stdout, "transaction_id: %d\n", res->transaction_id);
        fprintf(stdout, "connection_id: %lu\n", res->connection_id);
        if (req->transaction_id == res->transaction_id) {
            // Convert back to host endianness
            res->connection_id = htobe64(res->connection_id);
            res->action = htobe32(res->action);
            res->transaction_id = htobe32(res->transaction_id);
        } else {
            // Wrong server response
            return nullptr;
        }
    }
    free(req);
    return res;
}

void download(const char* raw_address) {
    address_t* split_addr = split_address(raw_address);
    char* ip = url_to_ip(*split_addr);
    int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd < 0) {
        // Error
        fprintf(stderr, "Socket creation failed");
        exit(2);
    }

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(decode_bencode_int(split_addr->port, nullptr));
    server_addr.sin_addr.s_addr = inet_addr(ip);
    connect_response_t* connect_response = connect_udp(&server_addr, sockfd, arc4random());
    free(connect_response);
    free(split_addr);
}