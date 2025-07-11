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
    if (announce_res_socket == nullptr) {
        fprintf(stderr, "Error while receiving announce response\n");
        return nullptr;
    }

    unsigned char buffer[MAX_RESPONSE_SIZE];
    const ssize_t recv_bytes = recvfrom(sockfd, buffer, MAX_RESPONSE_SIZE, 0, nullptr, &socklen);
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

void download(metainfo_t metainfo, const char* peer_id) {
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

    connection_data_t connection_data = {nullptr, nullptr, 0, nullptr};
    connection_id = connect_udp(counter, metainfo.announce_list, successful_index_pt, &connection_data);
    uint64_t downloaded = 0, left = metainfo.info->length, uploaded = 0;
    uint32_t event = 0, key = arc4random();

    announce_response_t* announce_response = announce_request_udp(connection_data.server_addr, connection_data.sockfd, connection_id, metainfo.info->pieces, peer_id, downloaded, left, uploaded, event, key, decode_bencode_int(connection_data.split_addr->port, nullptr));

    // Freeing announce response
    free(announce_response);
    // Freeing actually used connection
    free(connection_data.split_addr->host);
    free(connection_data.split_addr->port);
    free(connection_data.split_addr);
    free(connection_data.ip);
    free(connection_data.server_addr);

    //TODO After successful connection, proceed to download
}