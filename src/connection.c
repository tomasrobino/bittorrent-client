#include "connection.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <tgmath.h>
#include <time.h>

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

void shuffle_address_array(address_t* array[], const int length) {
    if (length > 1) {
        static unsigned int seed = 0;
        if (seed == 0) {
            seed = (unsigned int)time(nullptr);
        }

        for (size_t i = 0; i < length - 1; i++) {
            const size_t j = i + rand_r(&seed) / (RAND_MAX / (length - i) + 1);
            address_t* t = array[j];
            array[j] = array[i];
            array[i] = t;
        }
    }
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

int* try_request_udp(const int amount, const int sockfd[], const void *req[], const size_t req_size, const struct sockaddr *server_addr[]) {
    struct pollfd pfd[amount] = {0};
    for (int i = 0; i < amount; ++i) {
        pfd[i].fd = sockfd[i];
        pfd[i].events = POLLIN;
    }

    int counter = 0;
    int ret = 0;
    while (ret <= 0 && counter < 9) {
        // Send request
        for (int i = 0; i < amount; ++i) {
            const ssize_t sent = sendto(sockfd[i], req, req_size, 0, server_addr[i], sizeof(struct sockaddr));
            if (sent < 0) {
                // error
                fprintf(stderr, "Can't send connect request");
                exit(1);
            }
            fprintf(stdout, "Sent %zd bytes\n", sent);
        }

        // Wait for response
        ret = poll(pfd, 1, 15*pow(2, counter)*1000);
        if (ret > 0 && (pfd[0].revents & POLLIN)) {
            // Data ready
            int* sockfd_ret = malloc(sizeof(int)*ret);
            memset(sockfd_ret, 0, sizeof(int)*ret);
            for (int i = 0; i < amount; ++i) {
                if (pfd[i].revents & POLLIN) {
                    // Data available to be read on pfd[i].fd
                    sockfd_ret[i] = pfd[i].fd;
                }
            }
            return sockfd_ret;
        }
        if (ret == 0) {
            fprintf(stderr, "Timeout #%d\n", counter);
        } else {
            fprintf(stderr, "poll() error #%d\n", counter);
        }
        counter++;
    }
    fprintf(stderr, "Final timeout");
    return nullptr;
}

uint64_t connect_request_udp(const struct sockaddr *server_addr[], const int sockfd[], const int amount, int* successful_socket) {
    connect_request_t* req_array[amount] = {nullptr};
    for (int i = 0; i < amount; ++i) {
        req_array[i] = malloc(sizeof(connect_request_t));
        memset(req_array[i], 0, sizeof(connect_request_t));
        // Convert to network endianness
        req_array[i]->protocol_id = htobe64(0x41727101980LL);
        req_array[i]->action = htobe32(0);
        req_array[i]->transaction_id = htobe32(arc4random());
        fprintf(stdout, "Connection request:\n");
        fprintf(stdout, "action: %d\n", req_array[i]->action);
        fprintf(stdout, "transaction_id: %d\n", req_array[i]->transaction_id);
        fprintf(stdout, "protocol_id: %lu\n", req_array[i]->protocol_id);
    }

    int* available_connections = try_request_udp(amount, sockfd, req_array, sizeof(connect_request_t), server_addr);
    if (available_connections == nullptr) {
        // All connections failed
        for (int j = 0; j < amount; ++j) {
            free(req_array[j]);
        }
        return 0;
    }
    int i = 0;
    while (available_connections[i] != 0) {
        i++;
    }
    socklen_t socklen = sizeof(struct sockaddr);
    connect_response_t* res = malloc(sizeof(connect_response_t));
    recvfrom(sockfd[i], res, sizeof(connect_response_t), 0, nullptr, &socklen);

    fprintf(stdout, "Server response:\n");
    fprintf(stdout, "action: %d\n", res->action);
    fprintf(stdout, "transaction_id: %d\n", res->transaction_id);
    fprintf(stdout, "connection_id: %lu\n", res->connection_id);
    if (req_array[i]->transaction_id == res->transaction_id && req_array[i]->action == res->action) {
        // Convert back to host endianness
        res->connection_id = htobe64(res->connection_id);
        res->action = htobe32(res->action);
        res->transaction_id = htobe32(res->transaction_id);
    } else {
        // Wrong server response
        return 0;
    }

    for (int j = 0; j < amount; ++j) {
        free(available_connections);
        free(req_array[j]);
    }
    const uint64_t id = res->connection_id;
    free(res);
    if (successful_socket != nullptr) *successful_socket = sockfd[i];
    return id;
}

announce_response_t* announce_request_udp(const int amount, const struct sockaddr *server_addr[], const int sockfd[], uint64_t connection_id, char info_hash[], char peer_id[], const uint64_t downloaded, const uint64_t left, const uint64_t uploaded, const uint32_t event, const uint32_t key, const uint16_t port) {
    announce_request_t* req_array[amount];
    for (int i = 0; i < amount; ++i) {
        req_array[i] = malloc(sizeof(announce_request_t));
        memset(req_array[i], 0, sizeof(announce_request_t));
        // Convert to network endianness
        req_array[i]->connection_id = htobe64(connection_id);
        req_array[i]->action = htobe32(1);
        req_array[i]->transaction_id = htobe32(arc4random());
        strncpy(req_array[i]->info_hash, info_hash, 20);
        strncpy(req_array[i]->peer_id, peer_id, 20);
        req_array[i]->downloaded = htobe64(downloaded);
        req_array[i]->left = htobe64(left);
        req_array[i]->uploaded = htobe64(uploaded);
        req_array[i]->event = htobe32(event);
        req_array[i]->ip = htobe32(0);
        req_array[i]->key = htobe32(key);
        req_array[i]->num_want = htobe32(-1);
        req_array[i]->port = htobe16(port);

        fprintf(stdout, "Announce request:\n");
        fprintf(stdout, "action: %d\n", req_array[i]->action);
        fprintf(stdout, "transaction_id: %d\n", req_array[i]->transaction_id);
        fprintf(stdout, "connection_id: %lu\n", req_array[i]->connection_id);
        fprintf(stdout, "info_hash: ");
        for (int j = 0; j < 20; ++j) {
            fprintf(stdout, "%c",req_array[j]->info_hash[j]);
        }
        fprintf(stdout, "\n");
        fprintf(stdout, "peer_id: ");
        for (int j = 0; j < 20; ++j) {
            fprintf(stdout, "%d",req_array[j]->peer_id[j]);
        }
        fprintf(stdout, "\n");
        fprintf(stdout, "downloaded: %lu\n", req_array[i]->downloaded);
        fprintf(stdout, "left: %lu\n", req_array[i]->left);
        fprintf(stdout, "uploaded: %lu\n", req_array[i]->uploaded);
        fprintf(stdout, "key: %u\n", req_array[i]->key);
        fprintf(stdout, "port: %hu\n", req_array[i]->port);
    }

    announce_response_t* res = nullptr;
    socklen_t socklen = sizeof(struct sockaddr);
    const int* available_connections = try_request_udp(amount, sockfd, req_array, sizeof(announce_request_t), server_addr);
    if (available_connections == nullptr) {
        // All connections failed
        for (int j = 0; j < amount; ++j) {
            free(req_array[j]);
        }
        return nullptr;
    }
    int i = 0;
    while (available_connections[i] != 0) {
        i++;
    }

    char buffer[1500];
    const ssize_t recv_bytes = recvfrom(sockfd[i], buffer, sizeof(announce_response_t), 0, nullptr, &socklen);
    if (recv_bytes < 0) {
        fprintf(stderr, "No response");
        for (int j = 0; j < amount; ++j) {
            free(req_array[i]);
        }
        free(res);
        return nullptr;
    }
    memcpy(res, buffer, 20);


    int peer_size = 0;
    if (server_addr[i]->sa_family == AF_INET) {
        peer_size = 6;
    } else if (server_addr[i]->sa_family == AF_INET6) peer_size = 18;

    if (req_array[i]->transaction_id == res->transaction_id && req_array[i]->action == res->action) {
        for (int j = 0; j < amount; ++j) {
            free(req_array[i]);
        }
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
        for (int j = 0; j < amount; ++j) {
            free(req_array[i]);
        }
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
    // For storing socket that successfully connected
    int successful_socket = 0;
    int* successful_socket_pt = &successful_socket;
    // connection id from server response
    uint64_t connection_id;
    announce_list_ll* current = metainfo.announce_list;
    int counter = 0;
    // Get annnounce_list size
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
    // Creating outer list arrays
    address_t** split_addr_array[counter] = {nullptr};
    char** ip_array[counter] = {nullptr};
    int* sockfd_array[counter] = {nullptr};
    struct sockaddr* server_addr_array[counter];
    for (int i = 0; i < counter; ++i) {
        server_addr_array[i] = nullptr;
    }
    // Stores the size of each inner list
    int list_sizes[counter] = {0};

    counter = 0;
    int counter2 = 0;
    // Iterating over all lists
    while (current != nullptr) {
        ll* head = current->list;
        // Get current->list size
        while (current->list != nullptr) {
            current->list = current->list->next;
            counter2++;
        }
        current->list = head;
        list_sizes[counter] = counter2;

        // Allocating each inner list
        split_addr_array[counter] = malloc(sizeof(address_t*)*counter2);
        ip_array[counter] = malloc(sizeof(char*)*counter2);
        sockfd_array[counter] = malloc(sizeof(int*)*counter2);
        server_addr_array[counter] = malloc(sizeof(struct sockaddr*)*counter2);

        counter2 = 0;
        // Iterating over inner lists again
        while (current->list != nullptr) {
            // Splitting addresses
            split_addr_array[counter][counter2] = split_address(current->list->val);
            current->list = current->list->next;
            counter2++;
        }
        current->list = head;
        // Shuffling (done now to not overwrite metainfo.announce_list)
        shuffle_address_array(split_addr_array[counter], counter2);
        // Iterating over inner lists' newly-created arrays
        for (int i = 0; i < counter2; ++i) {
            // Gettign IPs
            ip_array[counter][counter2] = url_to_ip(split_addr_array[counter][counter2]);
            // Creating sockets
            sockfd_array[counter][counter2] = socket(split_addr_array[counter][counter2]->ip_version, SOCK_DGRAM, IPPROTO_UDP);

            // Creating sockaddr for each inner-list element
            if (split_addr_array[counter][counter2]->ip_version == AF_INET) {
                // For IPv4
                struct sockaddr_in server_addr = {0};
                server_addr.sin_family = split_addr_array[counter][counter2]->ip_version;
                server_addr.sin_port = htons(decode_bencode_int(split_addr_array[counter][counter2]->port, nullptr));
                server_addr.sin_addr.s_addr = inet_addr(ip_array[counter][counter2]);
            } else {
                // For IPv6
                struct sockaddr_in6 server_addr = {0};
                server_addr.sin6_family = split_addr_array[counter][counter2]->ip_version;
                server_addr.sin6_port = htons(decode_bencode_int(split_addr_array[counter][counter2]->port, nullptr));
                inet_pton(AF_INET6, ip_array[counter][counter2], &server_addr.sin6_addr);
            }
        }

        // Atempting connection of all trakcers in current list

        connection_id = connect_request_udp(server_addr_array, sockfd_array, list_sizes[counter], successful_socket_pt);
        if (connection_id != 0) {
            // Successful connection, exit loop
            break;
        }
        current = current->next;
        counter++;
    }
    current = metainfo.announce_list;

    // Memory cleanup
    for (int i = 0; i < counter; i++) {
        for (int j = 0; j < list_sizes[i]; j++) {
            // Free split_address allocations
            if (split_addr_array[i][j]) {
                free(split_addr_array[i][j]->host);
                free(split_addr_array[i][j]->port);
                free(split_addr_array[i][j]);
            }

            // Free IP strings
            if (ip_array[i][j]) {
                free(ip_array[i][j]);
            }
        }

        // Free the arrays themselves
        free(split_addr_array[i]);
        free(ip_array[i]);
        free(sockfd_array[i]);
        free(server_addr_array[i]);
    }

    //TODO After successful connection, proceed to download
}