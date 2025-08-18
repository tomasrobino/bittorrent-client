#include "messages.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>

#include "util.h"

void bitfield_to_hex(const unsigned char *bitfield, const unsigned int byte_amount, char *hex_output) {
    if (hex_output == NULL) return;
    for (int i = 0; i < byte_amount; i++) {
        sprintf(hex_output + i * 2, "%02x", bitfield[i]);
    }
    hex_output[byte_amount*2] = '\0';  // Null-terminate the string
}
int try_connect(const int sockfd, const struct sockaddr_in* peer_addr, const LOG_CODE log_code) {
    const int connect_result = connect(sockfd, (struct sockaddr*) peer_addr, sizeof(struct sockaddr));
    if (connect_result < 0 && errno != EINPROGRESS) {
        if (log_code >= LOG_ERR) fprintf(stderr, "Error #%d in connect for socket: %d\n", errno, sockfd);
        close(sockfd);
        return 0;
    }
    return 1;
}

int send_handshake(const int sockfd, const char* info_hash, const char* peer_id, const LOG_CODE log_code) {
    char buffer[HANDSHAKE_LEN] = {0};
    buffer[0] = 19;
    memcpy(buffer+1, "BitTorrent protocol", 19);
    memcpy(buffer+28, info_hash, 20);
    memcpy(buffer+48, peer_id, 20);

    // Send handshake request
    const ssize_t bytes_sent = send(sockfd, buffer, HANDSHAKE_LEN, MSG_NOSIGNAL);
    if (bytes_sent < 0) {
        if (log_code >= LOG_ERR) fprintf(stderr, "Error in handshake for socket: %d\n", sockfd);
    }
    return (int) bytes_sent;
}

char* handshake_response(const int sockfd, const char* info_hash, const LOG_CODE log_code) {
    // Receive response
    char* res = malloc(20);
    char res_buffer[HANDSHAKE_LEN];
    const ssize_t bytes_received = recv(sockfd, res_buffer, HANDSHAKE_LEN, 0);
    if (bytes_received < HANDSHAKE_LEN) {
        if (log_code >= LOG_ERR) fprintf(stderr, "Error in handshake for socket: %d\n", sockfd);
        close(sockfd);
        return nullptr;
    }

    if (memcmp(info_hash, res_buffer+28, 20) != 0) {
        if (log_code >= LOG_ERR) fprintf(stderr, "Error in handshake: returned wrong info hash\n");
        return nullptr;
    }
    memcpy(res, res_buffer+48, 20);
    return res;
}

unsigned char* process_bitfield(const unsigned char* client_bitfield, const unsigned char* foreign_bitfield, const unsigned int size) {
    const unsigned int byte_size = ceil(size/8.0);
    unsigned char* pending_bits = malloc(byte_size);
    for (int i = 0; i < byte_size; ++i) {
        pending_bits[i] = foreign_bitfield[i] & ~client_bitfield[i];
    }
    return pending_bits;
}

bittorrent_message_t* read_message(const int sockfd, time_t* peer_timestamp, const LOG_CODE log_code) {
    bittorrent_message_t* message = malloc(sizeof(bittorrent_message_t));
    memset(message, 0, sizeof(bittorrent_message_t));
    ssize_t bytes_received = recv(sockfd, message, MESSAGE_MIN_SIZE, 0);
    if (bytes_received < 5) {
        // Either keep-alive or error
        if (message->length == 0) {
            // keep-alive message, just update timestamp
            *peer_timestamp = time(nullptr);
        }
        free(message);
        return nullptr;
    }
    if (message->id < 0 || message->id > 9) {
        // Invalid id
        free(message);
        return nullptr;
    }
    *peer_timestamp = time(nullptr);
    message->length = htobe32(message->length);
    if (message->length-1 > 0) {
        message->payload = malloc(message->length-1);
        memset(message->payload, 0, message->length-1);
        long total = 0;
        while (total < message->length-1) {
            bytes_received = recv(sockfd, (message->payload)+total, message->length-1, 0);
            if (bytes_received == -1) {
                if (log_code >= LOG_ERR) fprintf(stderr, "Errno %d when attempting to read_message() on socket %d\n", errno, sockfd);
                break;
            }
            total+=bytes_received;
        }
        if (bytes_received < message->length-1) {
            return nullptr;
        }
    }
    return message;
}