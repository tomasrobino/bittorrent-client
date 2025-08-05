#include "messages.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/socket.h>

void bitfield_to_hex(const unsigned char *bitfield, const unsigned int byte_amount, char *hex_output) {
    if (hex_output == NULL) return;
    for (int i = 0; i < byte_amount; i++) {
        sprintf(hex_output + i * 2, "%02x", bitfield[i]);
    }
    hex_output[byte_amount*2] = '\0';  // Null-terminate the string
}
int try_connect(const int sockfd, const struct sockaddr_in* peer_addr) {
    const int connect_result = connect(sockfd, (struct sockaddr*) peer_addr, sizeof(struct sockaddr));
    if (connect_result < 0 && errno != EINPROGRESS) {
        fprintf(stderr, "Error #%d in connect for socket: %d\n", errno, sockfd);
        close(sockfd);
        return 0;
    }
    return 1;
}

int send_handshake(const int sockfd, const char* info_hash, const char* peer_id) {
    char buffer[HANDSHAKE_LEN] = {0};
    buffer[0] = 19;
    memcpy(buffer+1, "BitTorrent protocol", 19);
    memcpy(buffer+28, info_hash, 20);
    memcpy(buffer+48, peer_id, 20);

    // Send handshake request
    const ssize_t bytes_sent = send(sockfd, buffer, HANDSHAKE_LEN, 0);
    if (bytes_sent < 0) {
        fprintf(stderr, "Error in handshake for socket: %d\n", sockfd);
    }
    return (int) bytes_sent;
}

char* handshake_response(const int sockfd, const char* info_hash) {
    // Receive response
    char* res = malloc(20);
    char res_buffer[HANDSHAKE_LEN];
    const ssize_t bytes_received = recv(sockfd, res_buffer, HANDSHAKE_LEN, 0);
    if (bytes_received < HANDSHAKE_LEN) {
        fprintf(stderr, "Error in handshake for socket: %d\n", sockfd);
        close(sockfd);
        return nullptr;
    }

    if (memcmp(info_hash, res_buffer+28, 20) != 0) {
        fprintf(stderr, "Error in handshake: returned wrong info hash\n");
        return nullptr;
    }
    memcpy(res, res_buffer+48, 20);
    return res;
}

char* receive_bitfield(const int sockfd, const unsigned int amount) {
    const unsigned int byte_size = ceil(amount/8.0);
    bittorrent_message_t message = {0};
    ssize_t bytes_received = recv(sockfd, &message, MESSAGE_MIN_SIZE, 0);
    message.length = htobe32(message.length);
    if (message.length == byte_size+1 && message.id == BITFIELD) {
        message.payload = malloc(byte_size);
        bytes_received += recv(sockfd, message.payload, byte_size, 0);
        if (bytes_received > MESSAGE_MIN_SIZE ) {
            char* hex = malloc(byte_size*2 + 1);
            memset(hex, 0, byte_size*2 + 1);
            bitfield_to_hex((unsigned char*)message.payload, byte_size, hex);
            fprintf(stdout, "Received correct bitfield in socket %d: %s\n", sockfd, hex);
            free(hex);
            return message.payload;
        }
        free(message.payload);
    }
    fprintf(stderr, "Received erroneous bitfield in socket %d\n", sockfd);
    return nullptr;
}