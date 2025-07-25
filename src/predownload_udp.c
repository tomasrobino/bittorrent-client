#include "predownload_udp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <tgmath.h>
#include <time.h>
#include <unistd.h>

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
        //IPv6. Doesn't break, trying to get an IPv4, because WSL doesn't support IPv6 connections
        if (rp->ai_family == AF_INET6) {
            char buf[INET6_ADDRSTRLEN];
            addr_ptr = &((struct sockaddr_in6 *)rp->ai_addr)->sin6_addr;
            inet_ntop(rp->ai_family, addr_ptr, buf, sizeof(buf));
            ip = malloc(INET6_ADDRSTRLEN);
            if (ip) strcpy(ip, buf);
            printf("Resolved IPv6: %s\n", ip);
        }

        //IPv4
        if (rp->ai_family == AF_INET) {
            char buf[INET_ADDRSTRLEN];
            addr_ptr = &((struct sockaddr_in *)rp->ai_addr)->sin_addr;
            inet_ntop(rp->ai_family, addr_ptr, buf, sizeof(buf));
            ip = malloc(INET_ADDRSTRLEN);
            if (ip) strcpy(ip, buf);
            printf("Resolved IPv4: %s\n", ip);
            break;
        }
    }
    freeaddrinfo(res);
    return ip;
}

int* try_request_udp(const int amount, const int sockfd[], const void *req[], const size_t req_size, const struct sockaddr *server_addr[]) {
    struct pollfd pfd[amount];
    memset(pfd, 0, amount*sizeof(struct pollfd));
    for (int i = 0; i < amount; ++i) {
        pfd[i].fd = sockfd[i];
        pfd[i].events = POLLIN;
    }

    int counter = 0;
    int ret = 0;
    while (ret <= 0 && counter < MAX_ATTEMPTS) {
        // Send request
        for (int i = 0; i < amount; ++i) {
            const ssize_t sent = sendto(sockfd[i], req[i], req_size, 0, server_addr[i], sizeof(struct sockaddr));
            if (sent < 0) {
                fprintf(stderr, "Can't send request: %s (errno: %d)\n", strerror(errno), errno);
                for (int j = 0; j < amount; ++j) {
                    close(sockfd[j]);
                }
                return nullptr;
            }
            fprintf(stdout, "Sent %zd bytes\n", sent);
        }

        // Wait for response
        const int timeoutDuration = 15*pow(2, counter)*1000;
        ret = poll(pfd, amount, timeoutDuration);
        if (ret > 0 && pfd[0].revents & POLLIN) {
            // Data ready
            int* sockfd_ret = malloc(sizeof(int)*amount);
            memset(sockfd_ret, 0, sizeof(int)*ret);
            for (int i = 0; i < amount; ++i) {
                if (pfd[i].revents & POLLIN) {
                    // Data available to be read on pfd[i].fd
                    sockfd_ret[i] = pfd[i].fd;

                } else close(sockfd[i]);
            }
            return sockfd_ret;
        }
        if (ret == 0) {
            fprintf(stderr, "Timeout #%d, waited for %d seconds\n", counter+1, timeoutDuration/1000);
        } else {
            fprintf(stderr, "poll() error #%d\n", counter);
        }
        counter++;
    }
    fprintf(stderr, "Final timeout");
    return nullptr;
}

uint64_t connect_request_udp(const struct sockaddr *server_addr[], const int sockfd[], const int amount, int* successful_index) {
    connect_request_t* req_array[amount];
    for (int i = 0; i < amount; ++i) {
        req_array[i] = malloc(sizeof(connect_request_t));
        memset(req_array[i], 0, sizeof(connect_request_t));
        // Convert to network endianness
        req_array[i]->protocol_id = htobe64(0x41727101980LL);
        req_array[i]->action = htobe32(0);
        req_array[i]->transaction_id = htobe32(arc4random());
        fprintf(stdout, "Connection request:\n");
        fprintf(stdout, "action: %u\n", req_array[i]->action);
        fprintf(stdout, "transaction_id: %u\n", req_array[i]->transaction_id);
        fprintf(stdout, "protocol_id: %lu\n", req_array[i]->protocol_id);
    }

    int* available_connections = try_request_udp(amount, sockfd, (const void**)req_array, sizeof(connect_request_t), server_addr);
    if (available_connections == nullptr) {
        // All connections failed
        free(available_connections);
        for (int j = 0; j < amount; ++j) {
            free(req_array[j]);
        }
        return 0;
    }
    int i = 0;
    while (available_connections[i] == 0) {
        i++;
    }

    socklen_t socklen = sizeof(struct sockaddr);
    connect_response_t* res = malloc(sizeof(connect_response_t));
    const ssize_t received = recvfrom(sockfd[i], res, sizeof(connect_response_t), 0, nullptr, &socklen);
    if (received < 0) {
        fprintf(stderr, "Error while receiving connect response: %s (errno: %d)\n", strerror(errno), errno);
        return 0;
    }

    if (((error_response*) res)->action == 3) {
        // 3 means error
        fprintf(stderr, "Server returned error:\n");
        fprintf(stderr, "Transaction id: %d\n", ((error_response*) res)->transaction_id);
        fprintf(stderr, "Error message from the server: %s\n", ((error_response*) res)->message);
        free(res);
        return 0;
    }

    fprintf(stdout, "Received %ld bytes\n", received);

    fprintf(stdout, "Server response:\n");
    fprintf(stdout, "action: %u\n", res->action);
    fprintf(stdout, "transaction_id: %u\n", res->transaction_id);
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
        free(req_array[j]);
    }
    free(available_connections);
    const uint64_t id = res->connection_id;
    free(res);
    if (successful_index != nullptr) *successful_index = i;
    return id;
}

uint64_t connect_udp(const int amount, announce_list_ll* current, int* successful_index_pt, connection_data_t* connection_data) {
    int successful_index = *successful_index_pt;
    // Creating outer list arrays
    address_t** split_addr_array[amount];
    memset(split_addr_array, 0, amount*sizeof(address_t**));
    char** ip_array[amount];
    memset(ip_array, 0, amount*sizeof(char**));
    int* sockfd_array[amount];
    memset(sockfd_array, 0, amount*sizeof(int*));
    struct sockaddr** server_addr_array[amount];
    memset(server_addr_array, 0, amount*sizeof(struct sockaddr**));
    // Stores the size of each inner list
    int list_sizes[amount];
    memset(list_sizes, 0, amount*sizeof(int));

    int counter = 0;
    int counter2 = 0;
    // Iterating over all lists
    while (current != nullptr) {
        ll* list_head = current->list;
        // Get current->list size
        while (current->list != nullptr) {
            current->list = current->list->next;
            counter2++;
        }
        current->list = list_head;
        list_sizes[counter] = counter2;

        // Allocating each inner list
        split_addr_array[counter] = malloc(sizeof(address_t*)*counter2);
        memset(split_addr_array[counter], 0, counter2*sizeof(address_t*));
        ip_array[counter] = malloc(sizeof(char*)*counter2);
        memset(ip_array[counter], 0, counter2*sizeof(char*));
        sockfd_array[counter] = malloc(sizeof(int)*counter2);
        memset(sockfd_array[counter], 0, counter2*sizeof(int));
        server_addr_array[counter] = malloc(sizeof(struct sockaddr*)*counter2);
        memset(server_addr_array[counter], 0, counter2*sizeof(struct sockaddr*));

        counter2 = 0;
        // Iterating over inner lists again
        while (current->list != nullptr) {
            // Splitting addresses
            split_addr_array[counter][counter2] = split_address(current->list->val);
            current->list = current->list->next;
            counter2++;
        }
        current->list = list_head;
        // Shuffling (done now to not overwrite metainfo.announce_list)
        shuffle_address_array(split_addr_array[counter], counter2);
        // Iterating over inner lists' newly-created arrays
        for (int i = 0; i < counter2; ++i) {
            // Gettign IPs
            ip_array[counter][i] = url_to_ip(split_addr_array[counter][i]);
            // Creating sockets
            sockfd_array[counter][i] = socket(split_addr_array[counter][i]->ip_version, SOCK_DGRAM, IPPROTO_UDP);

            // Creating sockaddr for each inner-list element
            if (split_addr_array[counter][i]->ip_version == AF_INET) {
                // For IPv4
                struct sockaddr_in* server_addr = malloc(sizeof(struct sockaddr_in));
                server_addr->sin_family = split_addr_array[counter][i]->ip_version;
                server_addr->sin_port = htons(decode_bencode_int(split_addr_array[counter][i]->port, nullptr));
                server_addr->sin_addr.s_addr = inet_addr(ip_array[counter][i]);
                server_addr_array[counter][i] = (struct sockaddr*)server_addr;
            } else {
                // For IPv6
                struct sockaddr_in6* server_addr = malloc(sizeof(struct sockaddr_in6));
                server_addr->sin6_family = split_addr_array[counter][i]->ip_version;
                server_addr->sin6_port = htons(decode_bencode_int(split_addr_array[counter][i]->port, nullptr));
                inet_pton(AF_INET6, ip_array[counter][i], &server_addr->sin6_addr);
                server_addr_array[counter][i] = (struct sockaddr*)server_addr;
            }
        }

        // Atempting connection of all trackers in current list
        uint64_t connection_id = connect_request_udp((const struct sockaddr**)server_addr_array[counter], sockfd_array[counter], list_sizes[counter], successful_index_pt);
        if (connection_id != 0) {
            // Successful connection, exit loop

            // Memory cleanup of unused connections
            for (int i = 0; i < counter+1; i++) {
                // Free inner arrays
                for (int j = 0; j < list_sizes[i]; ++j) {
                    // Avoid freeing data actually in use
                    if (i == counter && j == successful_index) {
                        connection_data->split_addr = split_addr_array[i][j];
                        connection_data->ip = ip_array[i][j];
                        connection_data->sockfd = sockfd_array[i][j];
                        connection_data->server_addr = server_addr_array[i][j];
                    } else {
                        free(split_addr_array[i][j]->host);
                        free(split_addr_array[i][j]->port);
                        free(split_addr_array[i][j]);
                        free(ip_array[i][j]);
                        free(server_addr_array[i][j]);
                    }
                }
                // Free outer arrays
                free(split_addr_array[i]);
                free(ip_array[i]);
                free(sockfd_array[i]);
                free(server_addr_array[i]);
            }
            return connection_id;
        }
        current = current->next;
        counter++;
    }
    return 0;
}