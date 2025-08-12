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

int64_t calc_block_size(const unsigned int piece_size, const unsigned int byte_offset) {
    int64_t asked_bytes;
    // Amount of blocks in the piece
    const int64_t block_amount = (piece_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    // If last block
    if (block_amount-1 == byte_offset/BLOCK_SIZE) {
        asked_bytes = piece_size - BLOCK_SIZE * (block_amount-1);
    } else asked_bytes = BLOCK_SIZE;
    return asked_bytes;
}

char* get_path(const ll* filepath) {
    // Getting the amount of chars in the complete filepath
    int filepath_size = 0;
    const ll* filepath_ptr = filepath;
    while (filepath_ptr != nullptr) {
        // The +1 is for slashes and null terminator
        filepath_size += (int) strlen(filepath_ptr->val) + 1;
        filepath_ptr = filepath_ptr->next;
    }
    char* return_charpath = malloc(filepath_size);
    filepath_ptr = filepath;
    // Copying full path as string into *return_charpath
    struct stat st;
    while (filepath_ptr != nullptr) {
        strncpy(return_charpath + filepath_size, filepath_ptr->val, strlen(filepath_ptr->val));
        filepath_size += (int)strlen(filepath_ptr->val);
        return_charpath[filepath_size] = '/';

        // Creating directories
        if (filepath_ptr->next != nullptr && stat(return_charpath, &st) == -1) {
            // Doesn't exist, create it
            if (mkdir(return_charpath, 0755) == 0) {
                fprintf(stdout, "Created directory: %s", return_charpath);
            } else {
                fprintf(stderr, "Couldn't create directory: %s", return_charpath);
                exit(1);
            }
        }

        filepath_ptr = filepath_ptr->next;
    }
    return_charpath[filepath_size] = '\0';
    return return_charpath;
}

int32_t read_block_from_socket(const int sockfd, unsigned char* buffer, const int64_t amount) {
    // Reading data from socket
    int32_t total_received = 0;
    while (total_received < amount) {
        // this_file_ask can never be larger than BLOCK_SIZE, so narrowing conversion is fine
        const int32_t bytes_received = (int32_t) recv(sockfd, buffer, amount, 0);
        if (bytes_received < 1) {
            return -1;
        }
        total_received += bytes_received;
    }
    return total_received;
}

int32_t write_block(const unsigned char* buffer, const int64_t amount, FILE* file) {
    const int32_t bytes_written = (int32_t) fwrite(buffer, 1, amount, file);
    if (bytes_written != amount) {
        fprintf(stderr, "Failed to write to file %p\n", file);
        return -1;
    }
    fprintf(stdout, "Wrote %d bytes to file %p\n", bytes_written, file);
    return bytes_written;
}

int download_block(const int sockfd, const unsigned int piece_index, const unsigned int piece_size, const unsigned int byte_offset, files_ll* files_metainfo) {
    // Checking whether arguments are invalid
    if (byte_offset >= piece_size) return 4;
    if (piece_size == 0) return 4;

    int64_t byte_counter = (int64_t)piece_index*(int64_t)piece_size + (int64_t)byte_offset;
    // Actual amount of bytes the client's asking to download. Normally BLOCK_SIZE, but for the last block in a piece may be less
    /*
     * Maybe I'll turn this into a parameter instead
     */
    int64_t asked_bytes = calc_block_size(piece_size, byte_offset);
    // Buffer for recv()
    unsigned char buffer[BLOCK_SIZE];
    if (read_block_from_socket(sockfd, buffer, asked_bytes) < 0) {
        return 5;
    }
    int64_t buffer_offset = 0;

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
            char* filepath_char = get_path(current->path);
            // If file not open yet
            if (current->file_ptr == nullptr) {
                current->file_ptr = fopen(filepath_char, "rb+");
                // If the file doesn't exist, create it
                if (current->file_ptr == NULL) {
                    current->file_ptr = fopen(filepath_char, "wb+");
                }
                if (current->file_ptr == NULL) {
                    fprintf(stderr, "Failed to open file in download_block() for socket %d\n", sockfd);
                    free(filepath_char);
                    return 1;
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
            const int64_t bytes_written = write_block(buffer+buffer_offset, this_file_ask, current->file_ptr);
            if (bytes_written < 0) {
                // Error when writing
                free(filepath_char);
                return 3;
            }
            buffer_offset+=bytes_written;

            asked_bytes -= this_file_ask;
            byte_counter += this_file_ask;

            free(filepath_char);
        }
        current = current->next;
    }
    return 0;
}

bool piece_complete(const unsigned char *block_tracker, const unsigned int piece_index, const unsigned int piece_size, const int64_t torrent_size) {
    unsigned int this_piece_size = piece_size;
    if ( ((int64_t)piece_index+1) * (int64_t)piece_size > torrent_size ) {
        this_piece_size = torrent_size - (int64_t)piece_index * (int64_t)piece_size;
    }
    const unsigned int blocks_amount = ( (int64_t)this_piece_size+BLOCK_SIZE-1 ) / BLOCK_SIZE;
    const unsigned int first_block_global = (int64_t)piece_index * (( (int64_t)piece_size+BLOCK_SIZE-1 ) / BLOCK_SIZE);
    const unsigned int last_block_global = first_block_global + blocks_amount;

    for (unsigned int block_global = first_block_global; block_global < last_block_global; block_global++) {
        const unsigned int byte_index = block_global / 8;
        const unsigned int bit_offset = 7 - (block_global % 8);

        if ((block_tracker[byte_index] & (1u << bit_offset)) == 0) {
            // This block is not downloaded yet, piece incomplete
            return false;
        }
    }
    return true;
}

int torrent(const metainfo_t metainfo, const char* peer_id) {
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
    struct sockaddr_in* peer_addr_array = malloc(sizeof(struct sockaddr_in) * peer_amount);
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
        struct sockaddr_in* peer_addr = &peer_addr_array[counter2];
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
    const unsigned int bitfield_byte_size = ceil(metainfo.info->piece_number/8.0);
    unsigned char* bitfield = malloc(bitfield_byte_size);
    memset(bitfield, 0, bitfield_byte_size);
    // Size in bytes of the block tracker
    unsigned int block_tracker_bytesize = ceil(
        ceil(
            metainfo.info->piece_number*metainfo.info->piece_length / (double)BLOCK_SIZE
        ) / 8.0
    );
    // Actual amount of blocks per piece (not bytes)
    unsigned int blocks_per_piece = ceil(metainfo.info->piece_length / (double)BLOCK_SIZE);
    // Downloaded index for each block in a piece
    unsigned char* block_tracker = malloc(block_tracker_bytesize);
    memset(block_tracker, 0, block_tracker_bytesize);
    // Peer struct
    peer_t* peer_array = malloc(sizeof(peer_t)*peer_amount);
    memset(peer_array, 0, sizeof(peer_t)*peer_amount);
    for (int i = 0; i < peer_amount; ++i) {
        peer_array[i].socket = peer_socket_array[i];
    }

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
            peer_t peer = peer_array[index];

            // DEALING WITH CONNECTING
            // After calling connect()
            if (peer.status == PEER_NOTHING) {
                peer.status = PEER_CONNECTION_FAILURE;
                if (epoll_events[i].events & EPOLLOUT) {
                    int err = 0;
                    socklen_t len = sizeof(err);
                    // Check whether connect() was successful
                    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) < 0) {
                        fprintf(stderr, "Error in getspckopt() in socket %d\n", fd);
                    } else if (err != 0) {
                        fprintf(stderr, "Connection failed in socket %d\n", fd);
                    } else {
                        fprintf(stdout, "Connection successful in socket %d\n", fd);
                        peer.status = PEER_CONNECTION_SUCCESS;
                    }
                } else {
                    fprintf(stderr, "Connection in socket %d failed, EPOLLERR or EPOLLHUP\n", fd);
                }
            }
            // Retry connection if connect() failed
            if (peer.status == PEER_CONNECTION_FAILURE) {
                if (try_connect(fd, &peer_addr_array[index])) {
                    peer.status = PEER_NOTHING;
                }
                continue;
            }
            // Send handshake
            if (peer.status == PEER_CONNECTION_SUCCESS) {
                const int result = send_handshake(fd, metainfo.info->hash, peer_id);
                peer.last_msg = time(nullptr);
                if (result > 0) {
                    peer.status = PEER_HANDSHAKE_SENT;
                    fprintf(stdout, "Handshake sent through socket %d\n", fd);
                }
                continue;
            }
            // Receive handshake
            if (peer.status == PEER_HANDSHAKE_SENT && epoll_events[i].events & EPOLLIN) {
                const char* foreign_id = handshake_response(fd, metainfo.info->hash);
                peer.last_msg = time(nullptr);
                if (foreign_id != nullptr) {
                    peer.status = PEER_HANDSHAKE_SUCCESS;
                    peer.id = (char*)foreign_id;
                    fprintf(stdout, "Handshake successful in socket %d\n", fd);
                } else {
                    peer.status = PEER_CONNECTION_SUCCESS;
                }
                continue;
            }
            // Process messages
            if (peer.status >= PEER_HANDSHAKE_SUCCESS && epoll_events[i].events & EPOLLIN) {
                unsigned int byte_index = 0;
                unsigned int bit_offset = 0;
                const bittorrent_message_t* message = read_message(fd, &peer.last_msg);
                switch (message->id) {
                    case CHOKE:
                        peer.client_choked = true;
                        break;
                    case UNCHOKE:
                        peer.client_choked = false;
                        break;
                    case INTERESTED:
                        peer.peer_interest = true;
                        break;
                    case NOT_INTERESTED:
                        peer.peer_interest = false;
                        break;
                    case HAVE:
                        // Endianness
                        *message->payload = ntohl(*message->payload);
                        // Adding the new piece to the peer's bitfield
                        byte_index = *message->payload / 8;
                        bit_offset = 7 - *message->payload % 8;
                        peer.bitfield[byte_index] |= (1u << bit_offset);
                        break;
                    case BITFIELD:
                        peer.bitfield = message->payload;
                        if (message->payload != nullptr) {
                            fprintf(stdout, "Bitfield received successfully for socket %d\n", fd);
                        } else fprintf(stdout, "Error receiving bitfield for socket %d\n", fd);
                        break;
                    case REQUEST:
                        if (peer.peer_choked == false) {
                            request_t* request = (request_t*) message->payload;
                            // Endianness
                            request->index = ntohl(request->index);
                            request->begin = ntohl(request->begin);
                            request->length = ntohl(request->length);
                            byte_index = request->index / 8;
                            bit_offset = 7 - request->index % 8;
                            if (( peer.bitfield[byte_index] & (1u << bit_offset) ) != 0) {
                                // If this client has the requested piece
                                //TODO send requested piece
                            }
                        }
                        break;
                    case PIECE:
                        piece_t* piece = (piece_t*) message->payload;
                        // Endianness
                        piece->begin = ntohl(piece->begin);
                        piece->index = ntohl(piece->index);
                        byte_index = piece->index / 8;
                        bit_offset = 7 - piece->index % 8;
                        // If this client doesn't have the piece received
                        if ((bitfield[byte_index] & (1u << bit_offset)) == 0) {
                            // If this client doesn't have the block received
                            unsigned int global_block_index = piece->index * blocks_per_piece + piece->begin;
                            byte_index = global_block_index / 8;
                            bit_offset = 7 - global_block_index % 8;
                            if ( (block_tracker[byte_index] & (1u << bit_offset)) == 0) {
                                // If last piece, it's smaller
                                int64_t p_len;
                                if (piece->index == metainfo.info->piece_number-1) {
                                    // Conversion is fine beacuse single pieces aren't that large
                                    p_len = metainfo.info->length - piece->index*metainfo.info->piece_length;
                                } else p_len = metainfo.info->piece_length;
                                // ACTUAL DOWNLOAD
                                int block_result = download_block(fd, piece->index, p_len, piece->begin, metainfo.info->files);
                                if (block_result != 0) {
                                    exit(2);
                                }
                            } else fprintf(stderr, "Block received in socket %d belonging to piece %d already extant", fd, piece->index);
                        } else fprintf(stderr, "Piece received in socket %d already extant", fd);
                        break;
                    case CANCEL:
                        break;
                    case PORT:
                        break;
                    default: ;
                }
            }
        }
    }

    // Closing sockets
    close(epoll);
    for (int i = 0; i < peer_amount; ++i) {
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
        peer_ll* aux = announce_response->peer_list->next;
        free(announce_response->peer_list->ip);
        free(announce_response->peer_list);
        announce_response->peer_list = aux;
    }
    free(announce_response);

    return 0;
}