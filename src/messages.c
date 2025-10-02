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
    errno = 0;
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
    ssize_t total_sent = 0;
    errno = 0;
    while (total_sent < HANDSHAKE_LEN) {
        const ssize_t sent = send(sockfd, buffer+total_sent, HANDSHAKE_LEN-total_sent, MSG_NOSIGNAL);
        if (sent < 0) {
            if (log_code >= LOG_ERR) fprintf(stderr, "Error when sending handshake for socket: %d\n", sockfd);
        } else total_sent+=sent;
    }
    return (int) total_sent;
}

char* handshake_response(const int sockfd, const char* info_hash, const LOG_CODE log_code) {
    // Receive response
    char* res = malloc(20);
    char res_buffer[HANDSHAKE_LEN];
    ssize_t total_received = 0;
    errno = 0;
    while (total_received < HANDSHAKE_LEN) {
        const ssize_t bytes_received = recv(sockfd, res_buffer+total_received, HANDSHAKE_LEN-total_received, 0);
        if (bytes_received < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            if (log_code >= LOG_ERR) fprintf(stderr, "Error when receiving in handshake for socket: %d\n", sockfd);
            close(sockfd);
            return nullptr;
        }
        if (bytes_received > 0) total_received += bytes_received;
    }


    if (memcmp(info_hash, res_buffer+28, 20) != 0) {
        if (log_code >= LOG_ERR) fprintf(stderr, "Error in handshake: returned wrong info hash\n");
        return nullptr;
    }
    memcpy(res, res_buffer+48, 20);
    return res;
}

bool check_handshake(const unsigned char* info_hash, const unsigned char* buffer) {
    if (buffer[0] != 19) return false;
    if (memcmp(buffer+1, "BitTorrent protocol", 19) != 0) return false;
    if (memcmp(info_hash, buffer+28, 20) != 0) return false;
    return true;
}

unsigned char* process_bitfield(const unsigned char* client_bitfield, const unsigned char* foreign_bitfield, const unsigned int size) {
    const unsigned int byte_size = ceil(size/8.0);
    unsigned char* pending_bits = malloc(byte_size);
    for (int i = 0; i < byte_size; ++i) {
        pending_bits[i] = foreign_bitfield[i] & ~client_bitfield[i];
    }
    return pending_bits;
}
/*
bittorrent_message_t* read_message(const int sockfd, time_t* peer_timestamp, const LOG_CODE log_code) {
    bittorrent_message_t* message = malloc(sizeof(bittorrent_message_t));
    memset(message, 0, sizeof(bittorrent_message_t));
    ssize_t total_received = 0;
    errno = 0;
    while (total_received < MESSAGE_MIN_SIZE) {
        const ssize_t bytes_received = recv(sockfd, message+total_received, MESSAGE_MIN_SIZE-total_received, 0);
        if (bytes_received < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            if (log_code >= LOG_ERR) fprintf(stderr, "Error when reading message in socket: %d\n", sockfd);
            return nullptr;
        }
        if (bytes_received > 0) total_received += bytes_received;

        // keep-alive message, just update timestamp
        if (total_received == 4) {
            message->length = ntohl(message->length);
            if (message->length == 0) {
                *peer_timestamp = time(nullptr);
                free(message);
                return nullptr;
            }
        }
    }

    if (message->id < 0 || message->id > 9) {
        // Invalid id
        free(message);
        return nullptr;
    }
    *peer_timestamp = time(nullptr);
    if (message->length-1 > 0) {
        message->payload = malloc(message->length-1);
        memset(message->payload, 0, message->length-1);
        total_received = 0;
        errno = 0;
        while (total_received < message->length-1 ) {
            const ssize_t bytes_received = recv(sockfd, message->payload+total_received, message->length-1, 0);
            if (bytes_received < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                if (log_code >= LOG_ERR) fprintf(stderr, "Errno %d when attempting to read_message() in socket %d\n", errno, sockfd);
                free(message);
                return nullptr;
            }
            if (log_code == LOG_FULL) fprintf(stdout, "Read %ld bytes in read_message() in socket %d\n", bytes_received, sockfd);
            if (bytes_received > 0) total_received+=bytes_received;
        }
    }
    return message;
}
*/