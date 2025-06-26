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

    return req;
}

uint64_t connect_response_udp(connect_request_t* req, const int sockfd) {
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

announce_response_t* announce_request_udp(const struct sockaddr *server_addr, const int sockfd, uint64_t connection_id, char info_hash[], char peer_id[], const uint64_t downloaded, const uint64_t left, const uint64_t uploaded, const uint32_t event, const uint32_t key, const uint16_t port) {
    announce_request_t* req = malloc(sizeof(announce_request_t));
    memset(req, 0, sizeof(announce_request_t));
    // Convert to network endianness
    req->connection_id = htobe64(connection_id);
    req->action = htobe32(1);
    req->transaction_id = htobe32(arc4random());
    strncpy(req->info_hash, info_hash, 20);
    strncpy(req->peer_id, peer_id, 20);
    req->downloaded = htobe64(downloaded);
    req->left = htobe64(left);
    req->uploaded = htobe64(uploaded);
    req->event = htobe32(event);
    req->ip = htobe32(0);
    req->key = htobe32(key);
    req->num_want = htobe32(-1);
    req->port = htobe16(port);

    fprintf(stdout, "Announce request:\n");
    fprintf(stdout, "action: %d\n", req->action);
    fprintf(stdout, "transaction_id: %d\n", req->transaction_id);
    fprintf(stdout, "connection_id: %lu\n", req->connection_id);
    fprintf(stdout, "info_hash: ");
    for (int i = 0; i < 20; ++i) {
        fprintf(stdout, "%c",req->info_hash[i]);
    }
    fprintf(stdout, "\n");
    fprintf(stdout, "peer_id: ");
    for (int i = 0; i < 20; ++i) {
        fprintf(stdout, "%d",req->peer_id[i]);
    }
    fprintf(stdout, "\n");
    fprintf(stdout, "downloaded: %lu\n", req->downloaded);
    fprintf(stdout, "left: %lu\n", req->left);
    fprintf(stdout, "uploaded: %lu\n", req->uploaded);
    fprintf(stdout, "key: %u\n", req->key);
    fprintf(stdout, "port: %hu\n", req->port);


    const ssize_t sent = sendto(sockfd, req, sizeof(announce_request_t), 0, server_addr, sizeof(struct sockaddr));
    if (sent < 0) {
        // error
        fprintf(stderr, "Can't send announce request");
        exit(1);
    }
    fprintf(stdout, "Sent %zd bytes\n", sent);

    announce_response_t* res = malloc(sizeof(announce_response_t));
    memset(res, 0, sizeof(announce_response_t));
    socklen_t socklen = sizeof(struct sockaddr);

    char buffer[1500];
    const ssize_t recv_bytes = recvfrom(sockfd, buffer, sizeof(announce_response_t), 0, nullptr, &socklen);
    if (recv_bytes < 0) {
        fprintf(stderr, "No response");
        free(req);
        free(res);
        return nullptr;
    }
    memcpy(res, buffer, 20);


    int peer_size = 0;
    if (server_addr->sa_family == AF_INET) {
        peer_size = 6;
    } else if (server_addr->sa_family == AF_INET6) peer_size = 18;

    if (req->transaction_id == res->transaction_id && req->action == res->action) {
        free(req);
        // Convert back to host endianness
        res->action = htobe32(res->action);
        res->transaction_id = htobe32(res->transaction_id);
        res->interval = htobe32(res->interval);
        res->leechers = htobe32(res->leechers);
        res->seeders = htobe32(res->seeders);

        int peer_amount = (int)recv_bytes-20;
        peer_amount/=peer_size;
        if (peer_amount > 0) {
            peer_ll* head = malloc(sizeof(peer_ll));
            res->peer_list = head;
            int counter = 0;
            while (res->peer_list != nullptr) {
                res->peer_list->ip = malloc(sizeof(char)*(peer_size-2+1));
                memcpy(res->peer_list->ip, buffer+20 + peer_size*counter, peer_size-2);
                res->peer_list->ip[peer_size-2] = '\0';
                memcpy(res->peer_list, buffer+20 + peer_size-2 + peer_size*counter, 2);
                res->peer_list->port = htobe16(res->peer_list->port);
                if (counter<peer_amount-1) {
                    res->peer_list->next = malloc(sizeof(peer_ll));
                } else res->peer_list->next = nullptr;
                counter++;
            }
            res->peer_list = head;
        }
    } else {
        // Wrong server response
        free(req);
        free(res);
        return nullptr;
    }


    fprintf(stdout, "Server response:\n");
    fprintf(stdout, "action: %d\n", res->action);
    fprintf(stdout, "transaction_id: %d\n", res->transaction_id);
    fprintf(stdout, "interval: %u\n", res->interval);
    fprintf(stdout, "leechers: %u\n", res->leechers);
    fprintf(stdout, "seeders: %u\n", res->seeders);
    fprintf(stdout, "peer_list: \n");
    int counter = 0;
    while (res->peer_list != nullptr) {
        fprintf(stdout, "peer #%d: \n", counter);
        peer_ll* current = res->peer_list;
        while (current != nullptr) {
            fprintf(stdout, "id: %s\n",current->ip);
            fprintf(stdout, "port: %d\n",current->port);
            current = current->next;
        }
        counter++;
    }
    return res;
}

void download(metainfo_t metainfo) {
    announce_list_ll* current = metainfo.announce_list;
    int counter = 0;
    // Counts the trackers
    if (current != nullptr) {
        while (current != nullptr) {
            counter++;
            current = current->next;
        }
        current = metainfo.announce_list;
    } else {
        counter = 1;
        current = nullptr;
    }
    // Amount of lists of lists
    address_t** split_addr_array[counter] = {nullptr};
    char** ip_array[counter] = {nullptr};
    int* sockfd_array[counter] = {nullptr};
    int list_sizes[counter] = {0};
    counter = 0;
    int counter2 = 0;
    // Splitting addresses, getting IPs, and creating sockets
    while (current != nullptr) {
        ll* head = current->list;
        while (current->list != nullptr) {
            current->list = current->list->next;
            counter2++;
            list_sizes[counter] = counter2;
        }
        current->list = head;

        split_addr_array[counter] = malloc(sizeof(char*)*counter2);
        ip_array[counter] = malloc(sizeof(char*)*counter2);
        sockfd_array[counter] = malloc(sizeof(char*)*counter2);

        counter2 = 0;
        while (current->list != nullptr) {
            split_addr_array[counter][counter2] = split_address(current->list->val);
            ip_array[counter][counter2] = url_to_ip(split_addr_array[counter][counter2]);
            sockfd_array[counter][counter2] = socket(split_addr_array[counter][counter2]->ip_version, SOCK_DGRAM, IPPROTO_UDP);
            //char* ip = url_to_ip(split_addr);
            //int sockfd = socket(split_addr->ip_version, SOCK_DGRAM, IPPROTO_UDP);

            //TODO Try connecting to this group of trackers



            current->list = current->list->next;
            counter2++;
        }
        current->list = head;

        current = current->next;
        counter++;
    }
    current = metainfo.announce_list;

    //TODO After successful connection, proceed to download






    /*
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
        // const connect_request_t* request_result = try_request_udp((  void*(*)()  )connect_request_udp, sockfd, (struct sockaddr*)&server_addr, sockfd);
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
    */
}