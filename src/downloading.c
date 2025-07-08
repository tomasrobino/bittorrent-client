#include "downloading.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <bits/socket.h>
#include <sys/socket.h>

#include "connection.h"
#include "whole_bencode.h"

announce_response_t* announce_request_udp(const struct sockaddr *server_addr, const int sockfd, uint64_t connection_id, const char info_hash[], const char peer_id[], const uint64_t downloaded, const uint64_t left, const uint64_t uploaded, const uint32_t event, const uint32_t key, const uint16_t port) {
    announce_request_t req = {0};
    // Convert to network endianness
    req.connection_id = htobe64(connection_id);
    req.action = htobe32(1);
    req.transaction_id = htobe32(arc4random());
    strncpy(req.info_hash, info_hash, 20);
    req.info_hash[20] = '\0';
    strncpy(req.peer_id, peer_id, 20);
    req.peer_id[20] = '\0';
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


    socklen_t socklen = sizeof(struct sockaddr);
    const ssize_t sent = sendto(sockfd, &req, sizeof(announce_request_t), 0, server_addr, sizeof(struct sockaddr));
    if (sent < 0) {
        // error
        fprintf(stderr, "Can't send announce request: %s (errno: %d)\n", strerror(errno), errno);
        exit(1);
    }
    fprintf(stdout, "Sent %zd bytes\n", sent);

    char buffer[1500];
    const ssize_t recv_bytes = recvfrom(sockfd, buffer, sizeof(announce_response_t), 0, nullptr, &socklen);
    if (recv_bytes < 0) {
        fprintf(stderr, "Error while receiving connect response: %s (errno: %d)\n", strerror(errno), errno);
        return nullptr;
    }
    announce_response_t *res = malloc(recv_bytes);
    memcpy(res, buffer, 20);

    int peer_size = 0;
    if (server_addr->sa_family == AF_INET) {
        peer_size = 6;
    } else if (server_addr->sa_family == AF_INET6) peer_size = 18;

    if (req.transaction_id == res->transaction_id && req.action == res->action) {
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
    uint64_t downloaded = 0, left = 0, uploaded = 0;
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