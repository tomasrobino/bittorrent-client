#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "downloading.h"
#include "predownload_udp.h"
#include "whole_bencode.h"

char* handshake(const struct sockaddr *server_addr, int sockfd, const char* info_hash, const char* peer_id) {
    char buffer[68] = {0};
    buffer[0] = 19;
    memcpy(buffer+1, "BitTorrent protocol", 19);
    memcpy(buffer+28, info_hash, 20);
    memcpy(buffer+48, peer_id, 20);
    const socklen_t socklen = sizeof(struct sockaddr);
    // Try connecting
    fprintf(stdout, "Attempting to connect to peer\n");
    int connect_result = connect(sockfd, server_addr, socklen);
    if (connect_result < 0) {
        fprintf(stderr, "Error #%d in connect for socket: %d\n", errno, sockfd);
        close(sockfd);
        return nullptr;
    }

    // Send handshake request
    const ssize_t bytes_sent = send(sockfd, buffer, 68, 0);
    if (bytes_sent < 0) {
        fprintf(stderr, "Error in handshake for socket: %d", sockfd);
        close(sockfd);
        return nullptr;
    }

    // Receive response
    char* res = malloc(20);
    char res_buffer[68];
    const ssize_t bytes_received = recv(sockfd, res_buffer, 68, 0);
    if (bytes_received < 0) {
        fprintf(stderr, "Error in handshake for socket: %d", sockfd);
        close(sockfd);
        return nullptr;
    }

    if (memcmp(buffer+28, res_buffer+28, 20) != 0) {
        fprintf(stderr, "Error in handshake: returned wrong info hash");
        return nullptr;
    }
    memcpy(res, res_buffer+48, 20);
    return res;
}

int torrent(metainfo_t metainfo, const char* peer_id) {
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
    uint64_t downloaded = 0, left = metainfo.info->length, uploaded = 0;
    uint32_t event = 0, key = arc4random();

    connection_id = connect_udp(counter, metainfo.announce_list, successful_index_pt, &connection_data);
    if (connection_id == 0) {
        // Couldn't connect to any tracker
        return -1;
    }
    announce_response = announce_request_udp(connection_data.server_addr, connection_data.sockfd, connection_id, metainfo.info->hash, peer_id, downloaded, left, uploaded, event, key, decode_bencode_int(connection_data.split_addr->port, nullptr));
    if (announce_response == nullptr) {
        // Invalid response from tracker or error
        return -2;
    }

    // This is only to get torrent statistics
    //scrape_response_t* scrape_response = scrape_request_udp(connection_data.server_addr, connection_data.sockfd, connection_id, metainfo.info->hash, 1);

    // Freeing actually used UDP connection
    free(connection_data.split_addr->host);
    free(connection_data.split_addr->port);
    free(connection_data.split_addr);
    free(connection_data.ip);
    free(connection_data.server_addr);

    //TODO Actual download

    // Creating TCP sockets for all peers
    /*
        This only supports IPv4 for now
    */
    peer_ll* current_peer = announce_response->peer_list;
    // Getting amount of peers
    unsigned int peer_amount = 0;
    while (current_peer != nullptr) {
        peer_amount++;
        current_peer = current_peer->next;
    }
    current_peer = announce_response->peer_list;

    int* peer_socket_array = malloc(sizeof(int) * peer_amount);
    struct sockaddr_in** peer_addr_array = malloc(sizeof(struct sockaddr_in) * peer_amount);
    memset(peer_addr_array, 0, sizeof(struct sockaddr_in) * peer_amount);
    int counter2 = 0;
    while (current_peer != nullptr) {
        peer_socket_array[counter2] = socket(AF_INET, SOCK_STREAM, 0);
        if (peer_socket_array[counter2] == 0) {
            fprintf(stderr, "TCP socket creation failed");
            exit(1);
        }
        struct sockaddr_in* peer_addr = (struct sockaddr_in*) peer_addr_array+counter2;
        peer_addr->sin_family = AF_INET;
        peer_addr->sin_port = htons(current_peer->port);

        // Converting IP from string to binary
        if (inet_pton(AF_INET, current_peer->ip, &peer_addr->sin_addr) <= 0) {
            fprintf(stderr, "inet_pton failed while creating peer socket");
            close(peer_socket_array[counter2]);
            exit(1);
        }
        counter2++;
        current_peer = current_peer->next;
    }
    current_peer = announce_response->peer_list;
    char** peer_id_array = malloc(sizeof(char*) * peer_amount);
    memset(peer_id_array, 0, sizeof(char*) * peer_amount);
    for (int i = 0; i < peer_amount; ++i) {
        peer_id_array[i] = handshake((const struct sockaddr*) peer_addr_array+i, peer_socket_array[i], metainfo.info->hash, peer_id);
    }


    // Freeing peers
    free(peer_id_array);
    free(peer_socket_array);
    free(peer_addr_array);
    // Freeing announce response
    while (announce_response->peer_list != nullptr) {
        peer_ll* aux = announce_response->peer_list->next;
        free(announce_response->peer_list);
        announce_response->peer_list = aux;
    }
    free(announce_response);

    return 0;
}