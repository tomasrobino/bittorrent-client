#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <math.h>
#include <time.h>
#include <unistd.h>

#include "util.h"
#include "predownload_udp.h"

#include "basic_bencode.h"
#include "downloading_types.h"
#include "file.h"

address_t* split_address(const char* address) {
    if (!address) return nullptr;

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

void shuffle_address_array(address_t* array[], int32_t length) {
    if (!array) return;

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

char* url_to_ip(address_t* address, const LOG_CODE log_code) {
    struct addrinfo hints = {0}, *res;
    hints.ai_family = AF_UNSPEC;
    char* ip = nullptr;
    if (address->protocol == UDP) {
        hints.ai_socktype = SOCK_DGRAM;
    } else hints.ai_socktype = SOCK_STREAM;
    const int err = getaddrinfo(address->host, address->port, &hints, &res);
    if (err != 0) {
        if (log_code >= LOG_ERR) fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
        return nullptr;
    }

    // Iterating over received IPs
    for (const struct addrinfo *rp = res; rp != nullptr; rp = rp->ai_next) {
        void* addr_ptr;

        address->ip_version = rp->ai_family;
        //IPv6. Doesn't break, trying to get an IPv4, because WSL doesn't support IPv6 connections
        if (rp->ai_family == AF_INET6) {
            char buf[INET6_ADDRSTRLEN];
            addr_ptr = &((struct sockaddr_in6 *)rp->ai_addr)->sin6_addr;
            inet_ntop(rp->ai_family, addr_ptr, buf, sizeof(buf));
            ip = malloc(INET6_ADDRSTRLEN);
            if (ip) strcpy(ip, buf);
            if (log_code == LOG_FULL) fprintf(stdout, "Resolved IPv6: %s\n", ip);
        }

        //IPv4
        if (rp->ai_family == AF_INET) {
            char buf[INET_ADDRSTRLEN];
            addr_ptr = &((struct sockaddr_in *)rp->ai_addr)->sin_addr;
            inet_ntop(rp->ai_family, addr_ptr, buf, sizeof(buf));
            ip = malloc(INET_ADDRSTRLEN);
            if (ip) strcpy(ip, buf);
            if (log_code == LOG_FULL) fprintf(stdout, "Resolved IPv4: %s\n", ip);
            break;
        }
    }
    freeaddrinfo(res);
    return ip;
}

int32_t *try_request_udp(const int32_t amount, int32_t sockfd[], const void *req[], const size_t req_size, const struct sockaddr *server_addr[], const LOG_CODE log_code) {
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
                if (log_code >= LOG_ERR) fprintf(stderr, "Can't send request: %s (errno: %d)\n", strerror(errno), errno);
                for (int j = 0; j < amount; ++j) {
                    close(sockfd[j]);
                }
                return nullptr;
            }
            if (log_code == LOG_FULL) fprintf(stdout, "Sent %zd bytes\n", sent);
        }

        // Wait for response
        const int timeoutDuration = (int) (15*pow(2, counter)*1000);
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
            if (log_code >= LOG_ERR) fprintf(stderr, "Timeout #%d, waited for %d seconds\n", counter+1, timeoutDuration/1000);
        } else {
            if (log_code >= LOG_ERR) fprintf(stderr, "poll() error #%d\n", counter);
        }
        counter++;
    }
    if (log_code >= LOG_ERR) fprintf(stderr, "Final timeout");
    return nullptr;
}

uint64_t connect_request_udp(const struct sockaddr *server_addr[], int32_t sockfd[], const int32_t amount, int32_t *successful_index, const LOG_CODE log_code) {
    connect_request_t* req_array[amount];
    for (int i = 0; i < amount; ++i) {
        req_array[i] = malloc(sizeof(connect_request_t));
        memset(req_array[i], 0, sizeof(connect_request_t));
        // Convert to network endianness
        req_array[i]->protocol_id = htobe64(0x41727101980LL);
        req_array[i]->action = htobe32(0);
        req_array[i]->transaction_id = htobe32(arc4random());
        if (log_code >= LOG_SUMM) fprintf(stdout, "Connection request:\n");
        if (log_code >= LOG_SUMM) fprintf(stdout, "action: %u\n", req_array[i]->action);
        if (log_code >= LOG_SUMM) fprintf(stdout, "transaction_id: %u\n", req_array[i]->transaction_id);
        if (log_code >= LOG_SUMM) fprintf(stdout, "protocol_id: %lu\n", req_array[i]->protocol_id);
    }

    int* available_connections = try_request_udp(amount, sockfd, (const void**)req_array, sizeof(connect_request_t), server_addr, log_code);
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
        if (log_code >= LOG_ERR) fprintf(stderr, "Error while receiving connect response: %s (errno: %d)\n", strerror(errno), errno);
        free(res);
        return 0;
    }

    if (((error_response*) res)->action == 3) {
        // 3 means error
        if (log_code >= LOG_ERR) fprintf(stderr, "Server returned error:\n");
        if (log_code >= LOG_ERR) fprintf(stderr, "Transaction id: %d\n", ((error_response*) res)->transaction_id);
        if (log_code >= LOG_ERR) fprintf(stderr, "Error message from the server: %s\n", ((error_response*) res)->message);
        free(res);
        return 0;
    }

    if (log_code == LOG_FULL) fprintf(stdout, "Received %ld bytes\n", received);

    if (log_code >= LOG_SUMM) fprintf(stdout, "Server response:\n");
    if (log_code >= LOG_SUMM) fprintf(stdout, "action: %u\n", res->action);
    if (log_code >= LOG_SUMM) fprintf(stdout, "transaction_id: %u\n", res->transaction_id);
    if (log_code >= LOG_SUMM) fprintf(stdout, "connection_id: %lu\n", res->connection_id);
    if (req_array[i]->transaction_id == res->transaction_id && req_array[i]->action == res->action) {
        // Convert back to host endianness
        res->connection_id = htobe64(res->connection_id);
        res->action = htobe32(res->action);
        res->transaction_id = htobe32(res->transaction_id);
    } else {
        // Wrong server response
        free(res);
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

uint64_t connect_udp(int32_t amount, announce_list_ll* current, int32_t *successful_index_pt, connection_data_t* connection_data, const LOG_CODE log_code) {
    const int successful_index = *successful_index_pt;
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
            ip_array[counter][i] = url_to_ip(split_addr_array[counter][i], log_code);
            // Creating sockets
            sockfd_array[counter][i] = socket(split_addr_array[counter][i]->ip_version, SOCK_DGRAM, IPPROTO_UDP);

            // Creating sockaddr for each inner-list element
            if (split_addr_array[counter][i]->ip_version == AF_INET) {
                // For IPv4
                struct sockaddr_in* server_addr = malloc(sizeof(struct sockaddr_in));
                server_addr->sin_family = split_addr_array[counter][i]->ip_version;
                server_addr->sin_port = htons(decode_bencode_int(split_addr_array[counter][i]->port, nullptr, log_code));
                inet_pton(AF_INET, ip_array[counter][i], &server_addr->sin_addr);
                server_addr_array[counter][i] = (struct sockaddr*)server_addr;
            } else {
                // For IPv6
                struct sockaddr_in6* server_addr = malloc(sizeof(struct sockaddr_in6));
                server_addr->sin6_family = split_addr_array[counter][i]->ip_version;
                server_addr->sin6_port = htons(decode_bencode_int(split_addr_array[counter][i]->port, nullptr, log_code));
                inet_pton(AF_INET6, ip_array[counter][i], &server_addr->sin6_addr);
                server_addr_array[counter][i] = (struct sockaddr*)server_addr;
            }
        }

        // Atempting connection of all trackers in current list
        const uint64_t connection_id = connect_request_udp((const struct sockaddr**)server_addr_array[counter], sockfd_array[counter], list_sizes[counter], successful_index_pt, log_code);
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

announce_response_t *announce_request_udp(const struct sockaddr *server_addr, int32_t sockfd,
                                          const uint64_t connection_id, const unsigned char info_hash[],
                                          const unsigned char peer_id[], const torrent_stats_t* torrent_stats,
                                          const uint16_t port, const LOG_CODE log_code) {
    announce_request_t req = {0};
    // Convert to network endianness
    req.connection_id = htobe64(connection_id);
    req.action = htobe32(1);
    req.transaction_id = htobe32(arc4random());
    memcpy(req.info_hash, info_hash, 20);
    memcpy(req.peer_id, peer_id, 20);
    req.downloaded = htobe64(torrent_stats->downloaded);
    req.left = htobe64(torrent_stats->left);
    req.uploaded = htobe64(torrent_stats->uploaded);
    req.event = htobe32(torrent_stats->event);
    req.ip = htobe32(0);
    req.key = htobe32(torrent_stats->key);
    req.num_want = htobe32(-1);
    req.port = htobe16(port);

    if (log_code >= LOG_SUMM) fprintf(stdout, "Announce request:\n");
    if (log_code >= LOG_SUMM) fprintf(stdout, "action: %d\n", req.action);
    if (log_code >= LOG_SUMM) fprintf(stdout, "transaction_id: %d\n", req.transaction_id);
    if (log_code >= LOG_SUMM) fprintf(stdout, "connection_id: %lu\n", req.connection_id);
    if (log_code >= LOG_SUMM) fprintf(stdout, "info_hash: ");
    char human_hash[41];
    sha1_to_hex(req.info_hash, human_hash);
    if (log_code >= LOG_SUMM) fprintf(stdout, "%s", human_hash);
    if (log_code >= LOG_SUMM) fprintf(stdout, "\n");
    if (log_code >= LOG_SUMM) fprintf(stdout, "peer_id: ");
    for (int j = 0; j < 20; ++j) {
        if (log_code >= LOG_SUMM) fprintf(stdout, "%d",req.peer_id[j]);
    }
    if (log_code >= LOG_SUMM) fprintf(stdout, "\n");
    if (log_code >= LOG_SUMM) fprintf(stdout, "downloaded: %lu\n", req.downloaded);
    if (log_code >= LOG_SUMM) fprintf(stdout, "left: %lu\n", req.left);
    if (log_code >= LOG_SUMM) fprintf(stdout, "uploaded: %lu\n", req.uploaded);
    if (log_code >= LOG_SUMM) fprintf(stdout, "key: %u\n", req.key);
    if (log_code >= LOG_SUMM) fprintf(stdout, "port: %hu\n", req.port);
    // Explicit malloc to avoid sendto() error
    char* req_buffer = malloc(ANNOUNCE_REQUEST_SIZE);
    memcpy(req_buffer, &req.connection_id, 8);
    memcpy(req_buffer+8, &req.action, 4);
    memcpy(req_buffer+12, &req.transaction_id, 4);
    memcpy(req_buffer+16, &req.info_hash, 20);
    memcpy(req_buffer+36, &req.peer_id, 20);
    memcpy(req_buffer+56, &req.downloaded, 8);
    memcpy(req_buffer+64, &req.left, 8);
    memcpy(req_buffer+72, &req.uploaded, 8);
    memcpy(req_buffer+80, &req.event, 4);
    memcpy(req_buffer+84, &req.ip, 4);
    memcpy(req_buffer+88, &req.key, 4);
    memcpy(req_buffer+92, &req.num_want, 4);
    memcpy(req_buffer+96, &req.port, 2);


    int* announce_res_socket = try_request_udp(1, &sockfd, (const void**)&req_buffer, ANNOUNCE_REQUEST_SIZE, &server_addr, log_code);
    free(req_buffer);
    if (announce_res_socket == nullptr) {
        if (log_code >= LOG_ERR) fprintf(stderr, "Error while receiving announce response\n");
        free(announce_res_socket);
        return nullptr;
    }
    free(announce_res_socket);

    unsigned char buffer[MAX_RESPONSE_SIZE];
    const ssize_t recv_bytes = recvfrom(sockfd, buffer, MAX_RESPONSE_SIZE, 0, nullptr, nullptr);
    if (((error_response*) buffer)->action == 3) {
        // 3 means error
        if (log_code >= LOG_ERR) fprintf(stderr, "Server returned error:\n");
        if (log_code >= LOG_ERR) fprintf(stderr, "Transaction id: %d\n", ((error_response*) buffer)->transaction_id);
        if (log_code >= LOG_ERR) fprintf(stderr, "Error message from the server: %s\n", ((error_response*) buffer)->message);
        return nullptr;
    }

    if (recv_bytes < 0) {
        if (log_code >= LOG_ERR) fprintf(stderr, "Error while receiving announce response: %s (errno: %d)\n", strerror(errno), errno);
        return nullptr;
    }
    if (recv_bytes < 20) {
        if (log_code >= LOG_ERR) fprintf(stderr, "Invalid announce response\n");
        return nullptr;
    }
    announce_response_t *res = malloc(sizeof(announce_response_t));
    memcpy(res, buffer, 20);

    int peer_size = 0;
    if (server_addr->sa_family == AF_INET) {
        peer_size = 6;
    } else if (server_addr->sa_family == AF_INET6) peer_size = 18;

    peer_ll* head = nullptr;

    if (req.transaction_id == res->transaction_id && req.action == res->action) {
        // Convert back to host endianness
        res->action = htobe32(res->action);
        res->transaction_id = htobe32(res->transaction_id);
        res->interval = htobe32(res->interval);
        res->leechers = htobe32(res->leechers);
        res->seeders = htobe32(res->seeders);
        res->peer_list = nullptr;

        int peer_amount = (int)recv_bytes-20;
        peer_amount/=peer_size;
        if (peer_amount > 0) {
            head = malloc(sizeof(peer_ll));
            res->peer_list = head;
            int counter = 0;
            while (res->peer_list != nullptr) {
                /*
                    This only supports IPv4 for now
                */
                struct in_addr addr;
                memcpy(&addr.s_addr, buffer+20 + peer_size*counter, 4);
                // Checking if ip is 0
                if (addr.s_addr == 0) {
                    peer_amount--;
                    counter++;
                    continue;
                }
                res->peer_list->ip = malloc(INET_ADDRSTRLEN);
                inet_ntop(AF_INET, &addr, res->peer_list->ip, INET_ADDRSTRLEN);

                memcpy(&res->peer_list->port, buffer+20 + peer_size-2 + peer_size*counter, 2);
                res->peer_list->port = htobe16(res->peer_list->port);
                if (counter<peer_amount-1) {
                    res->peer_list->next = malloc(sizeof(peer_ll));
                } else res->peer_list->next = nullptr;
                res->peer_list = res->peer_list->next;
                counter++;
            }
            res->peer_list = head;
        }
    } else {
        // Wrong server response
        free(res);
        return nullptr;
    }

    if (log_code >= LOG_SUMM) fprintf(stdout, "Server response:\n");
    if (log_code >= LOG_SUMM) fprintf(stdout, "action: %u\n", res->action);
    if (log_code >= LOG_SUMM) fprintf(stdout, "transaction_id: %u\n", res->transaction_id);
    if (log_code >= LOG_SUMM) fprintf(stdout, "interval: %u\n", res->interval);
    if (log_code >= LOG_SUMM) fprintf(stdout, "leechers: %u\n", res->leechers);
    if (log_code >= LOG_SUMM) fprintf(stdout, "seeders: %u\n", res->seeders);
    if (log_code >= LOG_SUMM) fprintf(stdout, "peer_list: \n");
    int counter = 0;
    while (res->peer_list != nullptr) {
        if (log_code >= LOG_SUMM) fprintf(stdout, "peer #%d: %s:%d\n", counter+1, res->peer_list->ip, res->peer_list->port);
        counter++;
        res->peer_list = res->peer_list->next;
    }
    res->peer_list = head;
    return res;
}

scrape_response_t* scrape_request_udp(const struct sockaddr *server_addr, int32_t sockfd, const uint64_t connection_id, const char info_hash[], uint32_t
                                      torrent_amount, const LOG_CODE log_code) {
    scrape_request_t req;
    req.connection_id = htobe64(connection_id);
    req.action = htobe32(2);
    req.transaction_id = htobe32(arc4random());
    req.info_hash_list = info_hash;

    if (log_code >= LOG_SUMM) fprintf(stdout, "Scrape request:\n");
    if (log_code >= LOG_SUMM) fprintf(stdout, "action: %d\n", req.action);
    if (log_code >= LOG_SUMM) fprintf(stdout, "transaction_id: %d\n", req.transaction_id);
    if (log_code >= LOG_SUMM) fprintf(stdout, "connection_id: %lu\n", req.connection_id);
    if (log_code >= LOG_SUMM) fprintf(stdout, "info_hash: \n");
    for (int i = 0; i < torrent_amount; ++i) {
        if (log_code >= LOG_SUMM) fprintf(stdout, "#%d: ", i+1);
        for (int j = 0; j < 20; ++j) {
            if (log_code >= LOG_SUMM) fprintf(stdout, "%c",req.info_hash_list[j+20*i]);
        }
        if (log_code >= LOG_SUMM) fprintf(stdout, "\n");
    }
    char* buffer = malloc(SCRAPE_REQUEST_SIZE+20*torrent_amount);
    memcpy(buffer, &req, SCRAPE_REQUEST_SIZE);
    memcpy(buffer+SCRAPE_REQUEST_SIZE, req.info_hash_list, torrent_amount*20);
    socklen_t socklen = sizeof(struct sockaddr);
    int* scrape_res_socket = try_request_udp(1, &sockfd, (const void**)&buffer, SCRAPE_REQUEST_SIZE +20*torrent_amount, &server_addr, log_code);
    if (scrape_res_socket == nullptr) {
        if (log_code >= LOG_ERR) fprintf(stderr, "Error while receiving scrape response\n");
        free(scrape_res_socket);
        return nullptr;
    }
    free(scrape_res_socket);
    free(buffer);

    const unsigned int res_size = sizeof(scrape_response_t)+sizeof(scraped_data_t)*(torrent_amount-1);
    scrape_response_t* res = malloc(res_size);
    const ssize_t recv_bytes = recvfrom(sockfd, res, res_size, 0, nullptr, &socklen);
    if (((error_response*) res)->action == 3) {
        // 3 means error
        if (log_code >= LOG_ERR) fprintf(stderr, "Server returned error:\n");
        if (log_code >= LOG_ERR) fprintf(stderr, "Transaction id: %d\n", ((error_response*) res)->transaction_id);
        if (log_code >= LOG_ERR) fprintf(stderr, "Error message from the server: %s\n", ((error_response*) res)->message);
        free(res);
        return nullptr;
    }

    if (recv_bytes < 0) {
        if (log_code >= LOG_ERR) fprintf(stderr, "Error while receiving scrape response: %s (errno: %d)\n", strerror(errno), errno);
        free(res);
        return nullptr;
    }
    if (recv_bytes < 8) {
        if (log_code >= LOG_ERR) fprintf(stderr, "Invalid scrape response\n");
        free(res);
        return nullptr;
    }

    if (req.transaction_id == res->transaction_id && req.action == res->action) {
        // Convert back to host endianness
        res->action = htobe32(res->action);
        res->transaction_id = htobe32(res->transaction_id);
        for (int i = 0; i < torrent_amount; ++i) {
            res->scraped_data_array[i].seeders = htobe32(res->scraped_data_array[i].seeders);
            res->scraped_data_array[i].completed = htobe32(res->scraped_data_array[i].completed);
            res->scraped_data_array[i].leechers = htobe32(res->scraped_data_array[i].leechers);
        }
    } else {
        // Wrong server response
        free(res);
        return nullptr;
    }

    if (log_code >= LOG_SUMM) fprintf(stdout, "Server response:\n");
    if (log_code >= LOG_SUMM) fprintf(stdout, "action: %u\n", res->action);
    if (log_code >= LOG_SUMM) fprintf(stdout, "transaction_id: %u\n", res->transaction_id);
    if (log_code >= LOG_SUMM) fprintf(stdout, "scraped_data_array:\n");
    for (int i = 0; i < torrent_amount; ++i) {
        if (log_code >= LOG_SUMM) fprintf(stdout, "torrent #%d:\n", i+1);
        if (log_code >= LOG_SUMM) fprintf(stdout, "seeders: %d\n", res->scraped_data_array[i].seeders);
        if (log_code >= LOG_SUMM) fprintf(stdout, "completed: %d\n", res->scraped_data_array[i].completed);
        if (log_code >= LOG_SUMM) fprintf(stdout, "leechers: %d\n", res->scraped_data_array[i].leechers);
    }
    return res;
}