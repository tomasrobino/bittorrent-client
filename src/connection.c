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

char* url_to_ip(address_t* address) {
    struct addrinfo hints = {0}, *res;
    hints.ai_family = AF_UNSPEC;
    char* ip = nullptr;
    if (address->protocol == UDP) {
        hints.ai_socktype = SOCK_DGRAM;
    } else hints.ai_socktype = SOCK_STREAM;
    const int err = getaddrinfo(address->host, address->port, &hints, &res);
    if (err != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
        return nullptr;
    }

    // Iterating over received IPs
    for (struct addrinfo *rp = res; rp != nullptr; rp = rp->ai_next) {
        void* addr_ptr;

        address->ip_version = rp->ai_family;
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

uint64_t connect_request_udp(const struct sockaddr *server_addr, const int sockfd) {
    connect_request_t* req = malloc(sizeof(connect_request_t));
    memset(req, 0, sizeof(connect_request_t));
    // Convert to network endianness
    req->protocol_id = htobe64(0x41727101980LL);
    req->action = htobe32(0);
    req->transaction_id = htobe32(arc4random());
    fprintf(stdout, "Connection request:\n");
    fprintf(stdout, "action: %d\n", req->action);
    fprintf(stdout, "transaction_id: %d\n", req->transaction_id);
    fprintf(stdout, "protocol_id: %lu\n", req->protocol_id);

    const ssize_t sent = sendto(sockfd, req, sizeof(connect_request_t), 0, server_addr, sizeof(struct sockaddr));
    if (sent < 0) {
        // error
        fprintf(stderr, "Can't send connect request");
        exit(1);
    }
    fprintf(stdout, "Sent %zd bytes\n", sent);

    connect_response_t* res = malloc(sizeof(connect_response_t));
    socklen_t socklen = sizeof(struct sockaddr);
    const ssize_t recv_bytes = recvfrom(sockfd, res, sizeof(connect_response_t), 0, nullptr, &socklen);
    if (recv_bytes < 0) {
        fprintf(stderr, "No response");
        return 0;
    }

    fprintf(stdout, "Server response:\n");
    fprintf(stdout, "action: %d\n", res->action);
    fprintf(stdout, "transaction_id: %d\n", res->transaction_id);
    fprintf(stdout, "connection_id: %lu\n", res->connection_id);
    if (req->transaction_id == res->transaction_id && req->action == res->action) {
        // Convert back to host endianness
        res->connection_id = htobe64(res->connection_id);
        res->action = htobe32(res->action);
        res->transaction_id = htobe32(res->transaction_id);
    } else {
        // Wrong server response
        return 0;
    }
    free(req);
    const uint64_t id = res->connection_id;
    free(res);
    return id;
}

void download(const char* raw_address) {
    address_t* split_addr = split_address(raw_address);
    char* ip = url_to_ip(split_addr);
    int sockfd = socket(split_addr->ip_version, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd < 0) {
        // Error
        fprintf(stderr, "Socket creation failed\n");
        exit(2);
    }

    if (split_addr->ip_version == AF_INET) {
        struct sockaddr_in server_addr = {0};
        server_addr.sin_family = split_addr->ip_version;
        server_addr.sin_port = htons(decode_bencode_int(split_addr->port, nullptr));
        server_addr.sin_addr.s_addr = inet_addr(ip);
        const uint64_t connect_response = connect_request_udp((struct sockaddr*)&server_addr, sockfd);
        if (connect_response == 0) {
            // Error
            fprintf(stderr, "UDP connect request failed\n");
            exit(2);
        }
    } else {
        struct sockaddr_in6 server_addr = {0};
        server_addr.sin6_family = split_addr->ip_version;
        server_addr.sin6_port = htons(decode_bencode_int(split_addr->port, nullptr));
        inet_pton(AF_INET6, ip, &server_addr.sin6_addr);
        const uint64_t connect_response = connect_request_udp((struct sockaddr*)&server_addr, sockfd);
        if (connect_response == 0) {
            // Error
            fprintf(stderr, "UDP connect request failed\n");
            exit(2);
        }
    }

    free(split_addr);
}