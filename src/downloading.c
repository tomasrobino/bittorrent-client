#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "downloading.h"
#include "connection.h"
#include "whole_bencode.h"


announce_response_t* announce_request_udp(const struct sockaddr *server_addr, const int sockfd, uint64_t connection_id, const char info_hash[], const char peer_id[], const uint64_t downloaded, const uint64_t left, const uint64_t uploaded, const uint32_t event, const uint32_t key, const uint16_t port) {
    announce_request_t req = {0};
    // Convert to network endianness
    req.connection_id = htobe64(connection_id);
    req.action = htobe32(1);
    req.transaction_id = htobe32(arc4random());
    strncpy(req.info_hash, info_hash, 20);
    strncpy(req.peer_id, peer_id, 20);
    req.downloaded = htobe64(downloaded);
    req.left = htobe64(left);
    req.uploaded = htobe64(uploaded);
    req.event = htobe32(event);
    req.ip = htobe32(0);
    req.key = htobe32(key);
    req.num_want = htobe32(-1);
    req.port = htobe16(port);

    fprintf(stdout, "Announce request:\n");
    fprintf(stdout, "action: %d\n", req.action);
    fprintf(stdout, "transaction_id: %d\n", req.transaction_id);
    fprintf(stdout, "connection_id: %lu\n", req.connection_id);
    fprintf(stdout, "info_hash: ");
    for (int j = 0; j < 20; ++j) {
        fprintf(stdout, "%c",req.info_hash[j]);
    }
    fprintf(stdout, "\n");
    fprintf(stdout, "peer_id: ");
    for (int j = 0; j < 20; ++j) {
        fprintf(stdout, "%d",req.peer_id[j]);
    }
    fprintf(stdout, "\n");
    fprintf(stdout, "downloaded: %lu\n", req.downloaded);
    fprintf(stdout, "left: %lu\n", req.left);
    fprintf(stdout, "uploaded: %lu\n", req.uploaded);
    fprintf(stdout, "key: %u\n", req.key);
    fprintf(stdout, "port: %hu\n", req.port);
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


    socklen_t socklen = sizeof(struct sockaddr);
    int* announce_res_socket = try_request_udp(1, &sockfd, (const void**)&req_buffer, ANNOUNCE_REQUEST_SIZE, &server_addr);
    free(req_buffer);
    if (announce_res_socket == nullptr) {
        fprintf(stderr, "Error while receiving announce response\n");
        free(announce_res_socket);
        return nullptr;
    }
    free(announce_res_socket);

    unsigned char buffer[MAX_RESPONSE_SIZE];
    const ssize_t recv_bytes = recvfrom(sockfd, buffer, MAX_RESPONSE_SIZE, 0, nullptr, &socklen);
    if (((error_response*) buffer)->action == 3) {
        // 3 means error
        fprintf(stderr, "Server returned error:\n");
        fprintf(stderr, "Transaction id: %d\n", ((error_response*) buffer)->transaction_id);
        fprintf(stderr, "Error message from the server: %s\n", ((error_response*) buffer)->message);
        return nullptr;
    }

    if (recv_bytes < 0) {
        fprintf(stderr, "Error while receiving announce response: %s (errno: %d)\n", strerror(errno), errno);
        return nullptr;
    }
    if (recv_bytes < 20) {
        fprintf(stderr, "Invalid announce response\n");
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
                struct in_addr addr;
                memcpy(&addr.s_addr, buffer+20 + peer_size*counter, 4);
                // Checking if ip is 0
                if (addr.s_addr == 0) {
                    peer_amount--;
                    counter++;
                    continue;
                }
                res->peer_list->ip = inet_ntoa(addr);

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

    fprintf(stdout, "Server response:\n");
    fprintf(stdout, "action: %u\n", res->action);
    fprintf(stdout, "transaction_id: %u\n", res->transaction_id);
    fprintf(stdout, "interval: %u\n", res->interval);
    fprintf(stdout, "leechers: %u\n", res->leechers);
    fprintf(stdout, "seeders: %u\n", res->seeders);
    fprintf(stdout, "peer_list: \n");
    int counter = 0;
    while (res->peer_list != nullptr) {
        fprintf(stdout, "peer #%d: \n", counter+1);
        fprintf(stdout, "id: %s\n",res->peer_list->ip);
        fprintf(stdout, "port: %d\n",res->peer_list->port);
        counter++;
        res->peer_list = res->peer_list->next;
    }
    res->peer_list = head;
    return res;
}

scrape_response_t* scrape_request_udp(const struct sockaddr *server_addr, const int sockfd, const uint64_t connection_id, const char info_hash[], const unsigned int torrent_amount) {
    scrape_request_t req;
    req.connection_id = htobe64(connection_id);
    req.action = htobe32(2);
    req.transaction_id = htobe32(arc4random());
    req.info_hash_list = info_hash;

    fprintf(stdout, "Scrape request:\n");
    fprintf(stdout, "action: %d\n", req.action);
    fprintf(stdout, "transaction_id: %d\n", req.transaction_id);
    fprintf(stdout, "connection_id: %lu\n", req.connection_id);
    fprintf(stdout, "info_hash: \n");
    for (int i = 0; i < torrent_amount; ++i) {
        fprintf(stdout, "#%d: ", i+1);
        for (int j = 0; j < 20; ++j) {
            fprintf(stdout, "%c",req.info_hash_list[j+20*i]);
        }
        fprintf(stdout, "\n");
    }

    socklen_t socklen = sizeof(struct sockaddr);
    int* scrape_res_socket = try_request_udp(1, &sockfd, (const void**)&req, sizeof(scrape_request_t), &server_addr);
    if (scrape_res_socket == nullptr) {
        fprintf(stderr, "Error while receiving scrape response\n");
        free(scrape_res_socket);
        return nullptr;
    }
    free(scrape_res_socket);

    const unsigned int res_size = sizeof(scrape_response_t)+sizeof(scraped_data_t)*(torrent_amount-1);
    scrape_response_t* res = malloc(res_size);
    const ssize_t recv_bytes = recvfrom(sockfd, res, res_size, 0, nullptr, &socklen);
    if (((error_response*) res)->action == 3) {
        // 3 means error
        fprintf(stderr, "Server returned error:\n");
        fprintf(stderr, "Transaction id: %d\n", ((error_response*) res)->transaction_id);
        fprintf(stderr, "Error message from the server: %s\n", ((error_response*) res)->message);
        free(res);
        return nullptr;
    }

    if (recv_bytes < 0) {
        fprintf(stderr, "Error while receiving scrape response: %s (errno: %d)\n", strerror(errno), errno);
        free(res);
        return nullptr;
    }
    if (recv_bytes < 8) {
        fprintf(stderr, "Invalid scrape response\n");
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

    fprintf(stdout, "Server response:\n");
    fprintf(stdout, "action: %u\n", res->action);
    fprintf(stdout, "transaction_id: %u\n", res->transaction_id);
    fprintf(stdout, "scraped_data_array:\n");
    for (int i = 0; i < torrent_amount; ++i) {
        fprintf(stdout, "torrent #%d:\n", i+1);
        fprintf(stdout, "seeders: %d\n", res->scraped_data_array[i].seeders);
        fprintf(stdout, "completed: %d\n", res->scraped_data_array[i].completed);
        fprintf(stdout, "leechers: %d\n", res->scraped_data_array[i].leechers);
    }
    return res;
}

int download(metainfo_t metainfo, const char* peer_id) {
    // For storing socket that successfully connected
    int successful_index = 0;
    int* successful_index_pt = &successful_index;
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
    announce_response_t* announce_response;
    connection_data_t connection_data = {nullptr, nullptr, 0, nullptr};
    do {
        connection_id = connect_udp(counter, metainfo.announce_list, successful_index_pt, &connection_data);
        if (connection_id == 0) {
            // Couldn't connect to anyy tracker
            return -1;
        }
        uint64_t downloaded = 0, left = metainfo.info->length, uploaded = 0;
        uint32_t event = 0, key = arc4random();

        announce_response = announce_request_udp(connection_data.server_addr, connection_data.sockfd, connection_id, metainfo.info->hash, peer_id, downloaded, left, uploaded, event, key, decode_bencode_int(connection_data.split_addr->port, nullptr));
    } while (announce_response == nullptr);

    //TODO Actual download


    // Freeing announce response
    while (announce_response->peer_list != nullptr) {
        peer_ll* aux = announce_response->peer_list->next;
        free(announce_response->peer_list);
        announce_response->peer_list = aux;
    }
    free(announce_response);
    // Freeing actually used connection
    free(connection_data.split_addr->host);
    free(connection_data.split_addr->port);
    free(connection_data.split_addr);
    free(connection_data.ip);
    free(connection_data.server_addr);
    return 0;
}