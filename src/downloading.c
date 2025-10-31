#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <math.h>
#include <time.h>

#include "downloading.h"
#include "messages.h"
#include "predownload_udp.h"
#include "whole_bencode.h"

int64_t calc_block_size(const uint32_t piece_size, const uint32_t byte_offset) {
    int64_t asked_bytes;
    // Amount of blocks in the piece
    const int64_t block_amount = (piece_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    // If last block
    if (block_amount-1 == byte_offset/BLOCK_SIZE) {
        asked_bytes = piece_size - BLOCK_SIZE * (block_amount-1);
    } else asked_bytes = BLOCK_SIZE;
    return asked_bytes;
}

char* get_path(const ll* filepath, const LOG_CODE log_code) {
    // Getting the amount of chars in the complete filepath
    int32_t filepath_size = 0;
    const ll* filepath_ptr = filepath;
    while (filepath_ptr != nullptr) {
        // The +1 is for slashes and null terminator
        filepath_size += (int32_t) strlen(filepath_ptr->val) + 1;
        filepath_ptr = filepath_ptr->next;
    }
    char* return_charpath = malloc(filepath_size);
    memset(return_charpath, 0, filepath_size);
    filepath_size = 0;
    filepath_ptr = filepath;
    // Copying full path as string into *return_charpath
    struct stat st;
    while (filepath_ptr != nullptr) {
        memcpy(return_charpath + filepath_size, filepath_ptr->val, strlen(filepath_ptr->val));
        filepath_size += (int32_t)strlen(filepath_ptr->val);
        return_charpath[filepath_size] = '/';

        // Creating directories
        if (filepath_ptr->next != nullptr && stat(return_charpath, &st) == -1) {
            // Doesn't exist, create it
            if (mkdir(return_charpath, 0755) == 0) {
                if (log_code == LOG_FULL) fprintf(stdout, "Created directory: %s", return_charpath);
            } else {
                if (log_code >= LOG_ERR) fprintf(stderr, "Couldn't create directory: %s", return_charpath);
                exit(1);
            }
        }

        filepath_ptr = filepath_ptr->next;
    }
    return_charpath[filepath_size] = '\0';
    return return_charpath;
}

int32_t write_block(const unsigned char* buffer, const int64_t amount, FILE* file, const LOG_CODE log_code) {
    const int32_t bytes_written = (int32_t) fwrite(buffer, 1, amount, file);
    if (bytes_written != amount) {
        if (log_code >= LOG_ERR) if (log_code == LOG_FULL) fprintf(stdout, "Failed to write to file %p\n", file);
        return -1;
    }
    if (log_code >= LOG_ERR) fprintf(stderr, "Wrote %d bytes to file %p\n", bytes_written, file);
    return bytes_written;
}

int32_t process_block(const unsigned char *buffer, const uint32_t piece_size, files_ll *files_metainfo,
                      const LOG_CODE log_code) {
    const unsigned char* block = buffer+8;
    int32_t piece_index = 0;
    int32_t byte_offset = 0;
    memcpy(&piece_index, buffer, 4);
    memcpy(&byte_offset, buffer+4, 4);
    piece_index = (int32_t) ntohl(piece_index);
    byte_offset = (int32_t) ntohl(byte_offset);

    // Checking whether arguments are invalid
    if (byte_offset >= piece_size) return 1;
    if (piece_size == 0) return 1;

    int64_t byte_counter = (int64_t)piece_index*(int64_t)piece_size + (int64_t)byte_offset;
    // Actual amount of bytes the client's asking to download. Normally BLOCK_SIZE, but for the last block in a piece may be less
    /*
     * Maybe I'll turn this into a parameter instead
     */
    int64_t asked_bytes = calc_block_size(piece_size, byte_offset);
    int64_t block_offset = 0;

    // Finding out to which file the block belongs
    files_ll* current = files_metainfo;
    bool done = false;
    while (current != nullptr && !done) {
        // If the file starts before or at the block
        if (current->byte_index <= byte_counter && byte_counter < current->byte_index+current->length) {
            // To know how many bytes remain in this file
            const int64_t local_bytes = current->length - (byte_counter-current->byte_index);
            // If files_ll is malformed
            if (local_bytes <= 0) {
                current = current->next;
                continue;
            }
            char* filepath_char = get_path(current->path, log_code);
            // If file not open yet
            if (current->file_ptr == nullptr) {
                current->file_ptr = fopen(filepath_char, "rb+");
                // If the file doesn't exist, create it
                if (current->file_ptr == NULL) {
                    current->file_ptr = fopen(filepath_char, "wb+");
                }
                if (current->file_ptr == NULL) {
                    if (log_code >= LOG_ERR) fprintf(stderr, "Failed to open file in process_block() for piece %d, and offset %d\n", piece_index, byte_offset);
                    free(filepath_char);
                    return 2;
                }
            }

            // Getting how many bytes to read to this file
            int64_t this_file_ask;
            if (local_bytes >= asked_bytes) { // If the block ends before or at the same byte as the file
                this_file_ask = asked_bytes;
                // Since there are no other files in the block, done
                done = true;
            } else this_file_ask = local_bytes;
            // Advancing file pointer to proper position
            fseeko(current->file_ptr, current->length-local_bytes, SEEK_SET);

            // Writing to file
            const int64_t bytes_written = write_block(block+block_offset, this_file_ask, current->file_ptr, log_code);
            if (bytes_written < 0) {
                // Error when writing
                free(filepath_char);
                return 3;
            }
            block_offset+=bytes_written;

            asked_bytes -= this_file_ask;
            byte_counter += this_file_ask;

            free(filepath_char);
        }
        current = current->next;
    }
    return 0;
}

bool piece_complete(const unsigned char *block_tracker, const uint32_t piece_index, const uint32_t piece_size, const int64_t torrent_size) {
    uint32_t this_piece_size = piece_size;
    if ( ((int64_t)piece_index+1) * (int64_t)piece_size > torrent_size ) {
        this_piece_size = torrent_size - (int64_t)piece_index * (int64_t)piece_size;
    }
    const uint32_t blocks_amount = ( (int64_t)this_piece_size+BLOCK_SIZE-1 ) / BLOCK_SIZE;
    const uint32_t first_block_global = (int64_t)piece_index * (( (int64_t)piece_size+BLOCK_SIZE-1 ) / BLOCK_SIZE);
    const uint32_t last_block_global = first_block_global + blocks_amount;

    for (uint32_t block_global = first_block_global; block_global < last_block_global; block_global++) {
        const uint32_t byte_index = block_global / 8;
        const uint32_t bit_offset = 7 - (block_global % 8);

        if ((block_tracker[byte_index] & (1u << bit_offset)) == 0) {
            // This block is not downloaded yet, piece incomplete
            return false;
        }
    }
    return true;
}

bool are_bits_set(const unsigned char *bitfield, const uint32_t start, const uint32_t end) {
    const uint32_t start_byte = start / 8;
    const uint32_t end_byte = end / 8;
    const uint32_t start_bit = start % 8;
    const uint32_t end_bit = end % 8;

    // Mask for first byte
    unsigned char first_mask = (unsigned char)(0xFFu >> start_bit);
    first_mask &= (unsigned char)(0xFFu << (7u - ((start_byte == end_byte) ? end_bit : 7u)));

    // Mask for last byte
    unsigned char last_mask = (unsigned char)(0xFFu << (7u - end_bit));
    last_mask &= (unsigned char)(0xFFu >> ((start_byte == end_byte) ? start_bit : 0u));

    // Check first byte
    if ( (bitfield[start_byte] & first_mask) != first_mask )
        return false;

    // Full middle bytes
    for (uint32_t b = start_byte + 1; b < end_byte; b++) {
        if (bitfield[b] != 0xFFu)
            return false;
    }

    // Check last byte (if different from first)
    if (end_byte != start_byte) {
        if ( (bitfield[end_byte] & last_mask) != last_mask )
            return false;
    }

    return true;
}

void closing_files(const files_ll *files, const unsigned char *bitfield, const uint32_t piece_index,
                   const uint32_t piece_size, const uint32_t this_piece_size) {
    const uint32_t byte_index = piece_index / 8;
    const uint32_t bit_offset = 7 - piece_index % 8;
    // Checking whether the passed piece is actually downloaded
    if (( bitfield[byte_index] & (1u << bit_offset) ) == 0) {
        return;
    }
    bool last;
    int64_t piece_offset;
    if (piece_size != this_piece_size) {
        last = true;
        piece_offset = piece_index*(piece_size-1) + this_piece_size;
    } else {
        last = false;
        piece_offset = piece_index*piece_size;
    }

    const files_ll* current = files;
    while (current != nullptr) {
        // If the file ends after the piece starts and if it starts before the piece ends
        if (current->byte_index+current->length > piece_offset && current->byte_index < piece_offset+this_piece_size) {
            // If it overlaps with following pieces
            uint32_t right = 0;
            uint32_t left = 0;
            if (!last) {
                const int64_t overlap = piece_offset+this_piece_size - current->byte_index;
                right = ceil((double)(current->length - overlap)/(double)piece_size);
            }

            if (piece_index != 0) {
                const int64_t overlap = current->byte_index+current->length - piece_offset;
                left = ceil((double)(current->length - overlap) / (double)piece_size);
            }

            if (are_bits_set(bitfield, piece_index-left, piece_index+right)) {
                fclose(current->file_ptr);
            }
        }
        current = current->next;
    }
}

announce_response_t *handle_predownload_udp(const metainfo_t metainfo, const unsigned char *peer_id,
                                            const uint64_t downloaded, const uint64_t left, const uint64_t uploaded,
                                            const uint32_t event, const uint32_t key, const LOG_CODE log_code) {
    // For storing socket that successfully connected
    int32_t successful_index = 0;
    int32_t* successful_index_pt = &successful_index;
    // connection id from server response
    const announce_list_ll* current = metainfo.announce_list;
    int32_t counter = 0;
    // Get annnounce_list size
    if (current != nullptr) {
        while (current != nullptr) {
            counter++;
            current = current->next;
        }
    } else {
        counter = 1;
    }
    connection_data_t connection_data = {nullptr, nullptr, 0, nullptr};

    const uint64_t connection_id = connect_udp(counter, metainfo.announce_list, successful_index_pt, &connection_data, log_code);
    if (connection_id == 0) {
        // Couldn't connect to any tracker
        return nullptr;
    }
    announce_response_t *announce_response = announce_request_udp(connection_data.server_addr, connection_data.sockfd,
                                                                  connection_id, metainfo.info->hash, peer_id,
                                                                  downloaded, left, uploaded, event, key,
                                                                  decode_bencode_int(
                                                                      connection_data.split_addr->port, nullptr, log_code), log_code);
    if (announce_response == nullptr) {
        // Invalid response from tracker or error
        return nullptr;
    }

    // This is only to get torrent statistics
    //scrape_response_t* scrape_response = scrape_request_udp(connection_data.server_addr, connection_data.sockfd, connection_id, metainfo.info->hash, 1);

    // Freeing actually used UDP connection
    free(connection_data.split_addr->host);
    free(connection_data.split_addr->port);
    free(connection_data.split_addr);
    free(connection_data.ip);
    free(connection_data.server_addr);
    return announce_response;
}

bool read_from_socket(peer_t* peer, const int32_t epoll, const LOG_CODE log_code) {
    errno = 0;
    while (peer->reception_pointer < peer->reception_target && errno != EAGAIN && errno != EWOULDBLOCK ) {
        errno = 0;
        const ssize_t bytes_received = recv(peer->socket, peer->reception_cache+peer->reception_pointer, peer->reception_target-peer->reception_pointer, 0);
        if (bytes_received < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            if (log_code >= LOG_ERR) fprintf(stderr, "Error when reading message in socket: %d\n", peer->socket);
        }
        // Peer shutdown the connection. Shutting down my side too
        if (bytes_received == 0 || errno == ECONNRESET) {
            shutdown(peer->socket, SHUT_RDWR);
            epoll_ctl(epoll, EPOLL_CTL_DEL, peer->socket, nullptr);
            close(peer->socket);
            peer->status = PEER_CLOSED;
            peer->socket = -1;
            errno = 0;
            return false;
        }
        if (bytes_received > 0) {
            peer->reception_pointer += (int32_t)bytes_received;
        }
    }
    peer->last_msg = time(nullptr);
    errno = 0;
    return true;
}

uint32_t reconnect(peer_t* peer_list, const uint32_t peer_amount, uint32_t last_peer, const int32_t epoll, const LOG_CODE log_code) {
    for (int i = 0; i < peer_amount; ++i) {
        peer_t* peer = &peer_list[i];
        if (peer->status == PEER_CLOSED) {
            last_peer++;
            // Resetting peer
            peer->socket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
            // This could really be skipped. Here just in case
            memset(peer->reception_cache, 0, MAX_TRANS_SIZE);
            peer->reception_target = 0;
            peer->reception_pointer = 0;
            peer->am_choking = true;
            peer->am_interested = false;
            peer->peer_choking = true;
            peer->peer_interested = false;
            peer->bitfield = nullptr;
            peer->status = PEER_NOTHING;



            // Try connecting
            const int32_t connect_result = connect(peer->socket, (struct sockaddr*) peer->address, sizeof(struct sockaddr));
            if (connect_result < 0 && errno != EINPROGRESS) {
                if (log_code >= LOG_ERR) fprintf(stderr, "Error #%d in connect for socket: %d\n", errno, peer->socket);
                epoll_ctl(epoll, EPOLL_CTL_DEL, peer->socket, nullptr);
                close(peer->socket);
            } else if (errno == EINPROGRESS) {
                // If connection is in progress, add socket to epoll
                struct epoll_event ev;
                // EPOLLOUT means the connection attempt has finished, for good or ill
                ev.events = EPOLLIN | EPOLLOUT;
                ev.data.u32 = last_peer;
                epoll_ctl(epoll, EPOLL_CTL_ADD, peer->socket, &ev);
            }
        }
    }
    return last_peer;
}

int32_t torrent(const metainfo_t metainfo, const unsigned char *peer_id, const LOG_CODE log_code) {
    uint64_t downloaded = 0, left = metainfo.info->length, uploaded = 0;
    uint32_t event = 0, key = arc4random();
    announce_response_t* announce_response = handle_predownload_udp(metainfo, peer_id, downloaded, left, uploaded, event, key, log_code);
    if (announce_response == nullptr) return -1;
    // Creating TCP sockets for all peers
    /*
        This only supports IPv4 for now
    */
    peer_ll *current_peer = announce_response->peer_list;
    // Getting amount of peers
    uint32_t peer_amount = 0;
    while (current_peer != nullptr) {
        peer_amount++;
        current_peer = current_peer->next;
    }
    current_peer = announce_response->peer_list;

    int32_t *peer_socket_array = malloc(sizeof(int32_t) * peer_amount);
    struct sockaddr_in *peer_addr_array = malloc(sizeof(struct sockaddr_in) * peer_amount);
    memset(peer_addr_array, 0, sizeof(struct sockaddr_in) * peer_amount);
    int32_t counter2 = 0;
    // Creating epoll for controlling sockets
    const int32_t epoll = epoll_create1(0);
    while (current_peer != nullptr) {
        // Creating non-blocking socket
        peer_socket_array[counter2] = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
        if (peer_socket_array[counter2] <= 0) {
            if (log_code >= LOG_ERR) fprintf(stderr, "TCP socket creation failed");
            exit(1);
        }
        struct sockaddr_in *peer_addr = &peer_addr_array[counter2];
        peer_addr->sin_family = AF_INET;
        peer_addr->sin_port = htons(current_peer->port);

        // Converting IP from string to binary
        if (inet_pton(AF_INET, current_peer->ip, &peer_addr->sin_addr) <= 0) {
            if (log_code >= LOG_ERR) fprintf(stderr, "inet_pton failed while creating peer socket");
            close(peer_socket_array[counter2]);
            exit(1);
        }
        // Try connecting
        int32_t connect_result = connect(peer_socket_array[counter2], (struct sockaddr*) peer_addr, sizeof(struct sockaddr));
        if (connect_result < 0 && errno != EINPROGRESS) {
            if (log_code >= LOG_ERR) fprintf(stderr, "Error #%d in connect for socket: %d\n", errno, peer_socket_array[counter2]);
            close(peer_socket_array[counter2]);
        } else if (errno == EINPROGRESS) {
            // If connection is in progress, add socket to epoll
            struct epoll_event ev;
            // EPOLLOUT means the connection attempt has finished, for good or ill
            ev.events = EPOLLIN | EPOLLOUT;
            ev.data.u32 = counter2;
            epoll_ctl(epoll, EPOLL_CTL_ADD, peer_socket_array[counter2], &ev);
        }
        counter2++;
        current_peer = current_peer->next;
    }

    current_peer = announce_response->peer_list;
    // Checking connections with epoll
    struct epoll_event epoll_events[MAX_EVENTS];
    // General bitfield. Each piece takes up 1 bit
    const uint32_t bitfield_byte_size = ceil(metainfo.info->piece_number / 8.0);
    unsigned char *bitfield = malloc(bitfield_byte_size);
    if (!bitfield) return -1;
    memset(bitfield, 0, bitfield_byte_size);
    // Size in bytes of the block tracker
    uint32_t block_tracker_bytesize = ceil(
        ceil(
            metainfo.info->piece_number * metainfo.info->piece_length / (double) BLOCK_SIZE
        ) / 8.0
    );
    // Actual amount of blocks per piece (not bytes)
    uint32_t blocks_per_piece = ceil(metainfo.info->piece_length / (double) BLOCK_SIZE);
    // Downloaded index for each block in a piece
    unsigned char *block_tracker = malloc(block_tracker_bytesize);
    if (!block_tracker) return -1;
    memset(block_tracker, 0, block_tracker_bytesize);
    // Peer struct
    peer_t *peer_array = malloc(sizeof(peer_t) * peer_amount);
    if (!peer_array) return -1;
    memset(peer_array, 0, sizeof(peer_t) * peer_amount);
    for (int32_t i = 0; i < peer_amount; ++i) {
        peer_array[i].socket = peer_socket_array[i];
        // This could really be skipped. Here just in case
        memset(peer_array[i].reception_cache, 0, MAX_TRANS_SIZE);
        peer_array[i].reception_target = 0;
        peer_array[i].reception_pointer = 0;
        peer_array[i].am_choking = true;
        peer_array[i].am_interested = false;
        peer_array[i].peer_choking = true;
        peer_array[i].peer_interested = false;
        peer_array[i].bitfield = nullptr;
        peer_array[i].status = PEER_NOTHING;
    }

    /*
     *
     *  MAIN PEER INTERACTION LOOP
     *
     */
    while (left > 0) {
        const int32_t nfds = epoll_wait(epoll, epoll_events, MAX_EVENTS, EPOLL_TIMEOUT);
        if (nfds == -1) {
            if (log_code >= LOG_ERR) fprintf(stderr, "Error in epoll_wait\n");
            continue;
        }
        // No socket returned
        if (nfds == 0) {
            if (log_code >= LOG_ERR) fprintf(stderr, "Epoll timeout\n");
            continue;
        }

        for (int32_t i = 0; i < nfds; ++i) {
            const int32_t index = (int32_t) epoll_events[i].data.u32;
            peer_t *peer = &peer_array[index];

            // Fatal error in socket
            if (epoll_events[i].events == EPOLLERR) {
                int32_t err = 0;
                socklen_t len = sizeof(err);
                if (getsockopt(peer->socket, SOL_SOCKET, SO_ERROR, &err, &len) < 0) {
                    if (log_code >= LOG_ERR) fprintf(stderr, "Getsockopt error %d in socket %d\n", errno, peer->socket);
                } else if (err != 0) {
                    errno = err;
                    if (log_code >= LOG_ERR) fprintf(stderr, "Socket error %d in socket %d\n", errno, peer->socket);
                }
                epoll_ctl(epoll, EPOLL_CTL_DEL, peer->socket, nullptr);
                close(peer->socket);
                peer->status = PEER_CLOSED;
                peer->socket = -1;
                continue;
            }


            // DEALING WITH CONNECTING
            // After calling connect()
            if (peer->status == PEER_NOTHING) {
                peer->status = PEER_CONNECTION_FAILURE;
                if (epoll_events[i].events & EPOLLOUT) {
                    int32_t err = 0;
                    socklen_t len = sizeof(err);
                    // Check whether connect() was successful
                    if (getsockopt(peer->socket, SOL_SOCKET, SO_ERROR, &err, &len) < 0) {
                        if (log_code >= LOG_ERR) fprintf(stderr, "Error in getspckopt() in socket %d\n", peer->socket);
                    } else if (err != 0) {
                        if (log_code >= LOG_ERR) fprintf(stderr, "Connection failed in socket %d\n", peer->socket);
                    } else {
                        if (log_code == LOG_FULL) fprintf(stdout, "Connection successful in socket %d\n", peer->socket);
                        peer->status = PEER_CONNECTION_SUCCESS;
                    }
                } else {
                    if (log_code >= LOG_ERR) fprintf(stderr, "Connection in socket %d failed, EPOLLERR or EPOLLHUP\n",
                                                     peer->socket);
                }
            }
            // Retry connection if connect() failed
            if (peer->status == PEER_CONNECTION_FAILURE) {
                if (try_connect(peer->socket, &peer_addr_array[index], log_code)) {
                    if (errno != EINPROGRESS) {
                        epoll_ctl(epoll, EPOLL_CTL_DEL, peer->socket, nullptr);
                        close(peer->socket);
                        peer->status = PEER_CLOSED;
                        peer->socket = -1;
                    } else peer->status = PEER_NOTHING;
                }
                continue;
            }

            // Reading from socket
            read_from_socket(peer, epoll, log_code);

            // Send handshake
            if (peer->status == PEER_CONNECTION_SUCCESS && epoll_events[i].events & EPOLLOUT) {
                const int32_t result = send_handshake(peer->socket, metainfo.info->hash, peer_id, log_code);
                peer->last_msg = time(nullptr);
                if (result > 0) {
                    peer->status = PEER_HANDSHAKE_SENT;
                    if (log_code == LOG_FULL) fprintf(stdout, "Handshake sent through socket %d\n", peer->socket);
                    peer->reception_pointer = 0;
                    peer->reception_target = HANDSHAKE_LEN;
                } else {
                    if (log_code >= LOG_ERR) fprintf(stderr, "Error when sending handshake sent through socket %d\n",
                                                     peer->socket);
                    epoll_ctl(epoll, EPOLL_CTL_DEL, peer->socket, nullptr);
                    close(peer->socket);
                    peer->status = PEER_CLOSED;
                    peer->socket = -1;
                }
                continue;
            }
            // Check if handshake was received in full, and process it
            if (peer->status == PEER_HANDSHAKE_SENT && peer->reception_target == peer->reception_pointer) {
                const bool result = check_handshake(metainfo.info->hash, peer->reception_cache);
                if (result) {
                    peer->status = PEER_HANDSHAKE_SUCCESS;
                    peer->id = malloc(20);
                    memcpy(peer->id, peer->reception_cache + 48, 20);
                    peer->reception_pointer = 0;
                    peer->reception_target = MESSAGE_LENGTH_SIZE;
                    if (log_code == LOG_FULL) fprintf(stdout, "Handshake successful in socket %d\n", peer->socket);
                } else {
                    epoll_ctl(epoll, EPOLL_CTL_DEL, peer->socket, nullptr);
                    close(peer->socket);
                    peer->status = PEER_CLOSED;
                    peer->socket = -1;
                }
                memset(peer->reception_cache, 0, MAX_TRANS_SIZE);
            }

            // Send bitfield
            if ( (peer->status == PEER_HANDSHAKE_SUCCESS || peer->status == PEER_BITFIELD_RECEIVED)  && epoll_events[i].events & EPOLLOUT) {
                char *buffer = malloc(MESSAGE_LENGTH_AND_ID_SIZE + bitfield_byte_size);
                uint32_t length = 1 + bitfield_byte_size;
                length = htonl(length);
                memcpy(buffer, &length, MESSAGE_LENGTH_SIZE);
                buffer[MESSAGE_LENGTH_SIZE] = BITFIELD;
                memcpy(buffer + 5, bitfield, bitfield_byte_size);
                int64_t sent_bytes = 0;
                while (sent_bytes < MESSAGE_LENGTH_AND_ID_SIZE + bitfield_byte_size) {
                    int64_t sent = send(peer->socket, buffer + sent_bytes,
                                        MESSAGE_LENGTH_AND_ID_SIZE + bitfield_byte_size - sent_bytes, 0);
                    if (sent > 0) sent_bytes += sent;
                }

                free(buffer);
            }

            /*
             * Message reception
             */

            // Message length
            if (peer->status >= PEER_HANDSHAKE_SUCCESS && peer->reception_target == peer->reception_pointer && peer->reception_target == MESSAGE_LENGTH_SIZE) {
                if (read_message_length(peer->reception_cache, &peer->last_msg)) {
                    peer->reception_target = MESSAGE_LENGTH_AND_ID_SIZE;
                    peer->status = PEER_AWAITING_ID;
                } else {
                    // If message ended, be ready to receive or send the next message
                    peer->reception_target = MESSAGE_LENGTH_SIZE;
                    peer->reception_pointer = 0;
                }
                if (log_code == LOG_FULL) fprintf(stdout, "Peer %d received length\n", peer->socket);
            }

            // Message id
            if (peer->status >= PEER_AWAITING_ID && peer->reception_target == peer->reception_pointer && peer->reception_target == MESSAGE_LENGTH_AND_ID_SIZE) {
                bittorrent_message_t *message = (bittorrent_message_t *) peer->reception_cache;
                if (message->length > 1) {
                    // message has payload
                    peer->reception_target += (int32_t) message->length - 1;
                    peer->status = PEER_AWAITING_PAYLOAD;
                } else {
                    // message without payload
                    // If message ended, be ready to receive or send the next message
                    peer->reception_target = MESSAGE_LENGTH_SIZE;
                    peer->reception_pointer = 0;
                }
                if (log_code == LOG_FULL) fprintf(stdout, "Peer %d received id of %d with length of %d\n", peer->socket,
                                                  message->id, message->length);
            }

            // Message payload (if exists)
            if (peer->status >= PEER_AWAITING_PAYLOAD && peer->reception_target == peer->reception_pointer) {
                bittorrent_message_t *message = (bittorrent_message_t *) peer->reception_cache;
                message->payload = peer->reception_cache + 5;
                message->payload = peer->reception_cache + MESSAGE_LENGTH_AND_ID_SIZE;
                if (log_code == LOG_FULL) {
                    fprintf(stdout, "Peer %d received payload\n", peer->socket);
                    for (int k = 0; k < message->length - 1; ++k) {
                        fprintf(stdout, "%d|", message->payload[k]);
                    }
                    fprintf(stdout, "\n");
                }

                switch (message->id) {
                    case CHOKE:
                        peer->peer_choking = true;
                        break;
                    case UNCHOKE:
                        peer->peer_choking = false;
                        break;
                    case INTERESTED:
                        peer->peer_interested = true;
                        break;
                    case NOT_INTERESTED:
                        peer->peer_interested = false;
                        break;
                    case HAVE:
                        handle_have(peer, message->payload, bitfield, bitfield_byte_size, log_code);
                        break;
                    case BITFIELD:
                        handle_bitfield(peer, message->payload, bitfield, bitfield_byte_size, log_code);
                        break;
                    case REQUEST:
                        handle_request(peer, message->payload, log_code);
                        break;
                    case PIECE:
                        piece_t* piece = (piece_t*) message->payload;
                        // Endianness
                        piece->begin = ntohl(piece->begin);
                        piece->index = ntohl(piece->index);

                        const uint64_t download_size = handle_piece(peer, piece, metainfo, bitfield, block_tracker,
                                                                    blocks_per_piece, log_code);
                        downloaded += download_size;
                        left -= download_size;
                        broadcast_have(peer_array, peer_amount, piece->index, log_code);
                        break;
                    case CANCEL:
                    case PORT:
                        break;
                    default: ;
                }

                peer->reception_target = MESSAGE_LENGTH_SIZE;
                peer->reception_pointer = 0;
                memset(peer->reception_cache, 0, bitfield_byte_size);
                peer->status = PEER_HANDSHAKE_SUCCESS;
            }
        }


        for (int i = 0; i < peer_amount; ++i) {
            if (peer_array[i].status == PEER_CLOSED && difftime(time(nullptr), peer_array[i].last_msg) >= 10) {
                fprintf(stdout, "Attempting to reconnect socket #%d\n", peer_array[i].socket);
                reconnect(peer_array, peer_amount, counter2, epoll, log_code);
            }
        }
    }

    // Closing sockets
    close(epoll);
    for (int32_t i = 0; i < peer_amount; ++i) {
        close(peer_socket_array[i]);
    }
    // Freeing bitfield
    free(bitfield);
    free(block_tracker);
    // Freeing peer array
    free(peer_array);
    free(peer_socket_array);
    free(peer_addr_array);
    // Freeing announce response
    while (announce_response->peer_list != nullptr) {
        peer_ll *aux = announce_response->peer_list->next;
        free(announce_response->peer_list->ip);
        free(announce_response->peer_list);
        announce_response->peer_list = aux;
    }
    free(announce_response);

    return 0;
}
