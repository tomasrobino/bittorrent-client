#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>

#include "downloading.h"
#include "predownload_udp.h"
#include "whole_bencode.h"

int try_connect(const int sockfd, const struct sockaddr_in* peer_addr) {
    const int connect_result = connect(sockfd, (struct sockaddr*) peer_addr, sizeof(struct sockaddr));
    if (connect_result < 0 && errno != EINPROGRESS) {
        fprintf(stderr, "Error #%d in connect for socket: %d\n", errno, sockfd);
        close(sockfd);
        return 0;
    }
    return 1;
}

char* handshake(const struct sockaddr *server_addr, int sockfd, const char* info_hash, const char* peer_id) {
    char buffer[68] = {0};
    buffer[0] = 19;
    memcpy(buffer+1, "BitTorrent protocol", 19);
    memcpy(buffer+28, info_hash, 20);
    memcpy(buffer+48, peer_id, 20);

    // Send handshake request
    const ssize_t bytes_sent = send(sockfd, buffer, 68, 0);
    if (bytes_sent < 0) {
        fprintf(stderr, "Error in handshake for socket: %d\n", sockfd);
        close(sockfd);
        return nullptr;
    }

    // Receive response
    char* res = malloc(20);
    char res_buffer[68];
    const ssize_t bytes_received = recv(sockfd, res_buffer, 68, 0);
    if (bytes_received < 0) {
        fprintf(stderr, "Error in handshake for socket: %d\n", sockfd);
        close(sockfd);
        return nullptr;
    }

    if (memcmp(buffer+28, res_buffer+28, 20) != 0) {
        fprintf(stderr, "Error in handshake: returned wrong info hash\n");
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
    uint64_t downloaded = 0, left = metainfo.info->length, uploaded = 0;
    uint32_t event = 0, key = arc4random();

    const uint64_t connection_id = connect_udp(counter, metainfo.announce_list, successful_index_pt, &connection_data);
    if (connection_id == 0) {
        // Couldn't connect to any tracker
        return -1;
    }
    announce_response_t *announce_response = announce_request_udp(connection_data.server_addr, connection_data.sockfd,
                                                                  connection_id, metainfo.info->hash, peer_id,
                                                                  downloaded, left, uploaded, event, key,
                                                                  decode_bencode_int(
                                                                      connection_data.split_addr->port, nullptr));
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
    // Creating epoll for controlling sockets
    const int epoll = epoll_create1(0);
    while (current_peer != nullptr) {
        // Creating non-blocking socket
        peer_socket_array[counter2] = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
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
        // Try connecting
        //fprintf(stdout, "Attempting to connect to peer\n");
        int connect_result = connect(peer_socket_array[counter2], (struct sockaddr*) peer_addr, sizeof(struct sockaddr));
        if (connect_result < 0 && errno != EINPROGRESS) {
            fprintf(stderr, "Error #%d in connect for socket: %d\n", errno, peer_socket_array[counter2]);
            close(peer_socket_array[counter2]);
        } else if (errno == EINPROGRESS) {
            // If connection is in progress, add socket to epoll
            struct epoll_event ev;
            // EPOLLOUT means the connection attempt has finished, for good or ill
            ev.events = EPOLLOUT;
            ev.data.u32 = counter2;
            epoll_ctl(epoll, EPOLL_CTL_ADD, peer_socket_array[counter2], &ev);
        }
        counter2++;
        current_peer = current_peer->next;
    }

    current_peer = announce_response->peer_list;
    char** peer_id_array = malloc(sizeof(char*) * peer_amount);
    memset(peer_id_array, 0, sizeof(char*) * peer_amount);
    // Checking connections with epoll
    struct epoll_event epoll_events[MAX_EVENTS];
    PEER_STATUS* socket_status_array = malloc(sizeof(int)*peer_amount);
    memset(socket_status_array, 0, sizeof(int)*peer_amount);


    /*
     *
     *  MAIN PEER INTERACTION LOOP
     *
    */
    while (left > 0) {
        const int nfds = epoll_wait(epoll, epoll_events, MAX_EVENTS, EPOLL_TIMEOUT);
        if (nfds == -1) {
            fprintf(stderr, "Error in epoll_wait\n");
            continue;
        }
        // No socket returned
        if (nfds == 0) {
            fprintf(stderr, "Epoll timeout\n");
            continue;
        }

        for (int i = 0; i < nfds; ++i) {
            const int index = (int) epoll_events[i].data.u32;
            const int fd = peer_socket_array[index];

            if (socket_status_array[index] != PEER_NOTHING) { // After calling connect()
                if (epoll_events[i].events & EPOLLOUT) {
                    int err = 0;
                    socklen_t len = sizeof(err);
                    // Check whether connect() was successful
                    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) < 0) {
                        fprintf(stderr, "Error in getspckopt() in socket %d\n", fd);
                        socket_status_array[index] = PEER_NO_CONNECTION;
                    } else if (err != 0) {
                        fprintf(stderr, "Connection failed in socket %d\n", fd);
                        socket_status_array[index] = PEER_NO_CONNECTION;
                    } else {
                        fprintf(stdout, "Connection successful in socket %d\n", fd);
                        socket_status_array[index] = PEER_CONNECTED;
                    }
                } else {
                    fprintf(stderr, "Connection in socket %d failed, EPOLLERR or EPOLLHUP\n", fd);
                }
            } else if (socket_status_array[index] != PEER_NO_CONNECTION) { // Retry connection if connect() failed
                if (try_connect(fd, (struct sockaddr_in*)peer_addr_array+index)) {
                    socket_status_array[index] = PEER_NOTHING;
                }
            } else {

            }
        }
    }

    // Closing sockets
    close(epoll);
    for (int i = 0; i < peer_amount; ++i) {
        close(peer_socket_array[i]);
    }
    // Freeing socket status array
    free(socket_status_array);
    // Freeing peers
    free(peer_id_array);
    free(peer_socket_array);
    free(peer_addr_array);
    // Freeing announce response
    while (announce_response->peer_list != nullptr) {
        peer_ll* aux = announce_response->peer_list->next;
        free(announce_response->peer_list->ip);
        free(announce_response->peer_list);
        announce_response->peer_list = aux;
    }
    free(announce_response);

    return 0;
}