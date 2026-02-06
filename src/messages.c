#include "messages.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>

#include "downloading.h"
#include "util.h"

void bitfield_to_hex(const unsigned char *bitfield, const uint32_t byte_amount, char *hex_output) {
    if (hex_output == NULL) return;
    for (int32_t i = 0; i < byte_amount; i++) {
        sprintf(hex_output + i * 2, "%02x", bitfield[i]);
    }
    hex_output[byte_amount*2] = '\0';  // Null-terminate the string
}

int32_t try_connect(const int32_t sockfd, const struct sockaddr_in* peer_addr, const LOG_CODE log_code) {
    errno = 0;
    const int32_t connect_result = connect(sockfd, (struct sockaddr*) peer_addr, sizeof(struct sockaddr));
    if (connect_result < 0 && errno != EINPROGRESS) {
        if (log_code >= LOG_ERR) fprintf(stderr, "Error #%d in connect for socket: %d\n", errno, sockfd);
        close(sockfd);
        return 0;
    }
    return 1;
}

int32_t send_handshake(const int32_t sockfd, const unsigned char *info_hash, const unsigned char *peer_id, const LOG_CODE log_code) {
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
    return (int32_t) total_sent;
}

bool check_handshake(const unsigned char* info_hash, const unsigned char* buffer) {
    if (buffer[0] != 19) return false;
    if (memcmp(buffer+1, "BitTorrent protocol", 19) != 0) return false;
    if (memcmp(info_hash, buffer+28, 20) != 0) return false;
    return true;
}

unsigned char* process_bitfield(const unsigned char* client_bitfield, const unsigned char* foreign_bitfield, const uint32_t size) {
    if (size < 1) return nullptr;
    const uint32_t byte_size = ceil(size/8.0);
    unsigned char* pending_bits = malloc(byte_size);
    for (int32_t i = 0; i < byte_size; ++i) {
        pending_bits[i] = foreign_bitfield[i] & ~client_bitfield[i];
    }
    return pending_bits;
}

bool read_message_length(const unsigned char buffer[], time_t* peer_timestamp) {
    *peer_timestamp = time(nullptr);
    bittorrent_message_t* message = (bittorrent_message_t*)buffer;
    message->length = ntohl(message->length);
    // keep-alive message, just update timestamp
    if (message->length == 0) {
        free(message);
        return false;
    }
    // rest of messages
    return true;
}

void handle_have(peer_t *peer, const unsigned char *payload, const unsigned char *client_bitfield,
                 const uint32_t bitfield_byte_size, const LOG_CODE log_code) {
    // If the peer sends a HAVE without previously having sent a BITFIELD, create it
    if (peer->bitfield == nullptr) {
        peer->bitfield = malloc(bitfield_byte_size);
        memset(peer->bitfield, 0, bitfield_byte_size);
    }
    // Endianness
    uint32_t p_num = 0;
    memcpy(&p_num, payload, 4);
    p_num = ntohl(p_num);
    if (log_code == LOG_FULL) fprintf(stdout, "Received HAVE for piece %u in socket %d\n", p_num, peer->socket);
    // Adding the new piece to the peer's bitfield
    const uint32_t byte_index = p_num / 8;
    const uint32_t bit_offset = 7 - (p_num % 8);
    peer->bitfield[byte_index] |= (1u << bit_offset);
    // Checking my interest for peer's newly-downloaded piece
    if ( (~client_bitfield[byte_index] & peer->bitfield[byte_index]) != 0 ) {
        peer->am_interested = true;
    }
}

void handle_bitfield(peer_t *peer, const unsigned char *payload, const unsigned char *client_bitfield,
                     const uint32_t bitfield_byte_size, const LOG_CODE log_code) {
    if (payload == nullptr) return;

    peer->status = PEER_BITFIELD_RECEIVED;

    if (peer->bitfield == nullptr) {
        peer->bitfield = malloc(bitfield_byte_size);
    }

    if (payload != nullptr) {
        if (memcmp(peer->bitfield, client_bitfield, bitfield_byte_size) != 0) {
            memcpy(peer->bitfield, payload, bitfield_byte_size);
            int32_t j = 0;
            // Checking whether peer has any piece of interest
            while (!peer->am_interested && j < (int32_t)bitfield_byte_size) {
                if ((~client_bitfield[j] & peer->bitfield[j]) != 0) {
                    peer->am_interested = true;
                }
                j++;
            }
        }
        if (log_code == LOG_FULL) fprintf(stdout, "BITFIELD received successfully for socket %d\n", peer->socket);
    } else {
        memset(peer->bitfield, 0, bitfield_byte_size);
        if (log_code == LOG_FULL) fprintf(stdout, "Error receiving BITFIELD for socket %d\n", peer->socket);
    }
}

void handle_request(const peer_t* peer, unsigned char* payload, const LOG_CODE log_code) {
    if (peer->am_choking) return;

    request_t* request = (request_t*) payload;
    // Endianness
    request->index  = ntohl(request->index);
    request->begin  = ntohl(request->begin);
    request->length = ntohl(request->length);

    const uint32_t byte_index = request->index / 8;
    const uint32_t bit_offset = 7 - (request->index % 8);
    // If this client has the requested piece
    if ((peer->bitfield[byte_index] & (1u << bit_offset)) != 0) {
        // Constructing message buffer
        const uint32_t buffer_size = 5 + 8 + request->length;
        char* buffer = malloc(buffer_size);
        if (!buffer) return;

        uint32_t l = htonl(9 + request->length);
        memcpy(buffer, &l, 4);
        buffer[4] = 7; // PIECE id
        l = htonl(request->index);
        memcpy(buffer + 5, &l, 4);
        l = htonl(request->begin);
        memcpy(buffer + 9, &l, 4);

        // Sending block
        int32_t sent_bytes = 0;
        while (sent_bytes < buffer_size) {
            const int32_t sent = (int32_t)send(peer->socket, buffer + sent_bytes, buffer_size - sent_bytes, 0);
            if (sent < 0) {
                if (log_code >= LOG_ERR) fprintf(stderr, "Error while sending piece in socket %d", peer->socket);
                break;
            }
            sent_bytes += sent;
        }
        free(buffer);
    }
}

void broadcast_have(const peer_t* peer_array, const uint32_t peer_count, const uint32_t piece_index, const LOG_CODE log_code) {
    char* buffer = malloc(9);
    if (!buffer) return;

    for (int32_t j = 0; j < peer_count; ++j) {
        if (peer_array[j].status >= PEER_HANDSHAKE_SUCCESS) {
            // The five is the size of the id + index
            uint32_t l = htonl(5);
            memcpy(buffer, &l, MESSAGE_LENGTH_SIZE);
            buffer[MESSAGE_LENGTH_SIZE] = MESSAGE_LENGTH_SIZE;
            l = htonl(piece_index);
            memcpy(buffer + MESSAGE_LENGTH_AND_ID_SIZE, &l, MESSAGE_LENGTH_SIZE);

            int32_t sent_bytes = 0;
            while (sent_bytes < MESSAGE_LENGTH_AND_ID_SIZE + 4) {
                const int32_t res = (int32_t)send(peer_array[j].socket,
                                                  buffer + sent_bytes,
                                                  MESSAGE_LENGTH_AND_ID_SIZE + 4 - sent_bytes,
                                                  0);
                if (res == -1) {
                    if (log_code >= LOG_ERR) fprintf(stderr, "Error while sending have in socket %d", peer_array[j].socket);
                    break;
                }
                sent_bytes += res;
            }
        }
    }
    free(buffer);
}

int64_t write_block(const unsigned char* buffer, const uint64_t amount, FILE* file, const LOG_CODE log_code) {
    const uint32_t bytes_written = fwrite(buffer, 1, amount, file);
    if (bytes_written != amount) {
        if (log_code >= LOG_ERR) if (log_code == LOG_FULL) fprintf(stdout, "Failed to write to file %p\n", file);
        return -1;
    }
    if (log_code >= LOG_ERR) fprintf(stderr, "Wrote %d bytes to file %p\n", bytes_written, file);
    return bytes_written;
}

void free_ll_uint64_t(ll_uint64_t* ll) {
    ll_uint64_t* current = ll;
    while (current != nullptr) {
        ll_uint64_t* next = current->next;
        free(current);
        current = next;
    }
}

int32_t process_block(const piece_t *piece, const uint32_t standard_piece_size, const uint32_t this_piece_size,
                      files_ll *files_metainfo, const LOG_CODE log_code) {
    // Checking whether arguments are invalid
    if (!piece || !files_metainfo || !piece->block) return 1;
    if (piece->begin >= this_piece_size) return 1;
    if (standard_piece_size == 0) return 1;

    // The absolute index of the present byte in the whole torrent
    int64_t byte_counter = (int64_t)piece->index * (int64_t)standard_piece_size + (int64_t)piece->begin;
    // Actual amount of bytes the client's asking to download. Normally BLOCK_SIZE, but for the last block in a piece may be less
    int64_t asked_bytes = calc_block_size(this_piece_size, piece->begin);

    // Amount of files that the block touches
    uint32_t file_count = 0;
    // Linked list to hold the bytes to be written to each file the block touches
    ll_uint64_t* pending_bytes_head = malloc(sizeof(ll_uint64_t));
    if (!pending_bytes_head) return 1;

    pending_bytes_head->val = 0;
    pending_bytes_head->next = nullptr;
    ll_uint64_t* pending_bytes_current = pending_bytes_head;


    // Finding out to which file the block belongs
    files_ll* current = files_metainfo;
    // Additional header for when do relevant files start
    files_ll* first_touched_file = files_metainfo;
    bool done = false;
    while (current != nullptr && !done) {
        // If the block starts before the file ends
        if (byte_counter - current->byte_index < current->length) {
            first_touched_file = current;
            // To know how many bytes remain in this file
            const int64_t remaining_in_file = current->length - (byte_counter-current->byte_index);

            char* filepath_char = get_path(current->path, log_code);
            // If file not open yet
            {
                uint32_t count = 0;
                while (!current->file_ptr && count < MAX_FILE_ATTEMPTS) {
                    current->file_ptr = fopen(filepath_char, "rb+");
                    // If at first fopen() failed, try and try again
                    if (!current->file_ptr) {
                        if (errno == ENOENT) {
                            // If the file doesn't exist, create it
                            current->file_ptr = fopen(filepath_char, "wb+");
                        }
                    }
                    count++;
                }


                // Can't manage to open file
                if (!current->file_ptr) {
                    free(filepath_char);
                    free_ll_uint64_t(pending_bytes_head);
                    return 2;
                }
            }


            // Getting how many bytes to write to this file
            int64_t bytes_for_this_file;
            if (remaining_in_file >= asked_bytes) { // If the block ends before or at the same byte as the file
                bytes_for_this_file = asked_bytes;
                // Since there are no other files in the block, done
                done = true;
            } else bytes_for_this_file = remaining_in_file;
            // Advancing file pointer to proper position
            fseeko(current->file_ptr, byte_counter - current->byte_index, SEEK_SET);

            // Storing data for writing
            if (file_count != 0) {
                pending_bytes_current->next = malloc(sizeof(ll_uint64_t));
                if (!pending_bytes_current->next) {
                    free(filepath_char);
                    free_ll_uint64_t(pending_bytes_head);
                    return 1;
                }
                pending_bytes_current = pending_bytes_current->next;
            }
            pending_bytes_current->val = bytes_for_this_file;
            pending_bytes_current->next = nullptr;
            file_count++;


            // Updating counters
            asked_bytes -= bytes_for_this_file;
            byte_counter += bytes_for_this_file;

            free(filepath_char);
        }
        current = current->next;
    }

    // Critical error. Should never happen
    if (asked_bytes != 0) {
        free_ll_uint64_t(pending_bytes_head);
        return 4;
    }

    pending_bytes_current = pending_bytes_head;
    current = first_touched_file;


    int64_t block_offset = 0;
    // Writing to file

    // TODO allow me to revert partial block writes


    // TODO THIS SHOULD BE ON A DIFFERENT THREAD FROM torrent()

    for (uint32_t i = 0; i < file_count; ++i) {
        const int64_t bytes_written = write_block(piece->block+block_offset, pending_bytes_current->val, current->file_ptr, log_code);
        if (bytes_written < 0) {
            // Error when writing
            free_ll_uint64_t(pending_bytes_head);
            return 3;
        }
        pending_bytes_current = pending_bytes_current->next;
        current = current->next;
        block_offset += bytes_written;
    }

    free_ll_uint64_t(pending_bytes_head);
    return 0;
}

uint64_t handle_piece(const piece_t* piece, const uint32_t socket, const metainfo_t metainfo,
                      unsigned char* client_bitfield, unsigned char* block_tracker, const uint32_t blocks_per_piece,
                      const LOG_CODE log_code) {
    // Initializing variables and converting endianness
    const uint32_t p_begin = ntohl(piece->begin);
    const uint32_t p_index = ntohl(piece->index);
    
    uint32_t byte_index = p_index / 8;
    uint32_t bit_offset = 7 - (p_index % 8);
    // If this client already has the piece received
    if ((client_bitfield[byte_index] & (1u << bit_offset)) != 0) {
        if (log_code >= LOG_ERR) fprintf(stderr, "Piece received in socket %d already extant", socket);
        return 0;
    }
    // If this client already has the block received
    const uint32_t global_block_index = p_index * blocks_per_piece + p_begin;
    byte_index = global_block_index / 8;
    bit_offset = 7 - (global_block_index % 8);
    if ((block_tracker[byte_index] & (1u << bit_offset)) != 0) {
        if (log_code >= LOG_ERR) fprintf(stderr, "Block received in socket %d belonging to piece %d already extant", socket, p_index);
        return 0;
    }

    // If last piece, it's smaller
    int64_t this_piece_length;
    if (p_index == metainfo.info->piece_number - 1) {
        this_piece_length = metainfo.info->length - (int64_t)p_index * (int64_t)metainfo.info->piece_length;
    } else this_piece_length = metainfo.info->piece_length;


    // DOWNLOAD
    const int32_t block_result = process_block(piece, metainfo.info->piece_length, this_piece_length, metainfo.info->files, log_code);
    if (block_result != 0) return 0;

    const uint64_t this_block = calc_block_size(this_piece_length, p_begin);
    // Update block tracker
    block_tracker[byte_index] |= (1u << bit_offset);
    // If all the blocks in a piece are downloaded, mark it in the bitfield and prepare
    // to send "have" message to all peer_array
    if (piece_complete(block_tracker, p_index, metainfo.info->piece_length, metainfo.info->length)) {
        const uint32_t p_byte_index = p_index / 8;
        const uint32_t p_bit_offset = 7 - (p_index % 8);
        client_bitfield[p_byte_index] |= (1u << p_bit_offset);

        closing_files(metainfo.info->files, client_bitfield, p_index, metainfo.info->piece_length, (uint32_t)this_piece_length);
    }

    return this_block;
}
