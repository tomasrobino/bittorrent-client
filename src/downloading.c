#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <math.h>
#include <time.h>

#include "downloading.h"

#include "basic_bencode.h"
#include "disk.h"
#include "predownload_udp.h"
#include "parsing.h"
#include "messages.h"

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
    if (!filepath) return nullptr;
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

        if (filepath_ptr->next != nullptr) {
            return_charpath[filepath_size] = '/';
            filepath_size++;
        }

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

bool piece_complete(const unsigned char *block_tracker, const uint32_t piece_index, const uint32_t piece_size, const int64_t torrent_size) {
    if (!block_tracker) return false;

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
    if (!bitfield || start > end) return false;

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

announce_response_t *handle_predownload_udp(const metainfo_t metainfo, const unsigned char *peer_id, const torrent_stats_t* torrent_stats, const LOG_CODE log_code) {
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
                                                                  torrent_stats, decode_bencode_int(
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
    if (!peer || epoll < 0) return false;

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
    if (!peer_list || epoll < 0) return 0;

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

uint32_t count_peers(const announce_response_t *announce_response) {
    uint32_t peer_amount = 0;
    const peer_ll *current_peer = announce_response->peer_list;
    while (current_peer != nullptr) {
        peer_amount++;
        current_peer = current_peer->next;
    }
    return peer_amount;
}

void free_announce_response(announce_response_t *announce_response) {
    if (!announce_response) return;

    while (announce_response->peer_list != nullptr) {
        peer_ll *aux = announce_response->peer_list->next;
        free(announce_response->peer_list->ip);
        free(announce_response->peer_list);
        announce_response->peer_list = aux;
    }
    free(announce_response);
}

void cleanup_torrent_ctx(torrent_ctx_t *ctx) {
    if (!ctx) return;

    if (ctx->epoll >= 0) {
        close(ctx->epoll);
    }
    if (ctx->peer_socket_array) {
        for (uint32_t i = 0; i < ctx->peer_amount; ++i) {
            if (ctx->peer_socket_array[i] >= 0) {
                close(ctx->peer_socket_array[i]);
            }
        }
    }

    free(ctx->bitfield);
    free(ctx->block_tracker);
    free(ctx->peer_array);
    free(ctx->peer_socket_array);
    free(ctx->peer_addr_array);
    free(ctx->torrent_stats);
    free_announce_response(ctx->announce_response);
}

bool init_torrent_stats_and_tracker(const metainfo_t metainfo, const unsigned char *peer_id,
                                    const LOG_CODE log_code, torrent_ctx_t *ctx) {
    ctx->torrent_stats = malloc(sizeof(torrent_stats_t));
    if (!ctx->torrent_stats) return false;
    ctx->torrent_stats->downloaded = 0;
    ctx->torrent_stats->left = metainfo.info->length;
    ctx->torrent_stats->uploaded = 0;
    ctx->torrent_stats->event = 0;
    ctx->torrent_stats->key = arc4random();

    ctx->announce_response = handle_predownload_udp(metainfo, peer_id, ctx->torrent_stats, log_code);
    return ctx->announce_response != nullptr;
}

bool init_peer_connections(torrent_ctx_t *ctx, const LOG_CODE log_code) {
    ctx->peer_amount = count_peers(ctx->announce_response);
    ctx->peer_socket_array = malloc(sizeof(int32_t) * ctx->peer_amount);
    ctx->peer_addr_array = malloc(sizeof(struct sockaddr_in) * ctx->peer_amount);
    if (!ctx->peer_socket_array || !ctx->peer_addr_array) return false;

    memset(ctx->peer_addr_array, 0, sizeof(struct sockaddr_in) * ctx->peer_amount);
    for (uint32_t i = 0; i < ctx->peer_amount; ++i) {
        ctx->peer_socket_array[i] = -1;
    }

    ctx->epoll = epoll_create1(0);
    if (ctx->epoll < 0) return false;

    peer_ll *current_peer = ctx->announce_response->peer_list;
    int32_t counter = 0;
    while (current_peer != nullptr) {
        ctx->peer_socket_array[counter] = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
        if (ctx->peer_socket_array[counter] <= 0) {
            if (log_code >= LOG_ERR) fprintf(stderr, "TCP socket creation failed");
            return false;
        }

        struct sockaddr_in *peer_addr = &ctx->peer_addr_array[counter];
        peer_addr->sin_family = AF_INET;
        peer_addr->sin_port = htons(current_peer->port);
        if (inet_pton(AF_INET, current_peer->ip, &peer_addr->sin_addr) <= 0) {
            if (log_code >= LOG_ERR) fprintf(stderr, "inet_pton failed while creating peer socket");
            close(ctx->peer_socket_array[counter]);
            ctx->peer_socket_array[counter] = -1;
            return false;
        }

        int32_t connect_result = connect(ctx->peer_socket_array[counter], (struct sockaddr*)peer_addr, sizeof(struct sockaddr));
        if (connect_result < 0 && errno != EINPROGRESS) {
            if (log_code >= LOG_ERR) fprintf(stderr, "Error #%d in connect for socket: %d\n", errno, ctx->peer_socket_array[counter]);
            close(ctx->peer_socket_array[counter]);
            ctx->peer_socket_array[counter] = -1;
        } else if (errno == EINPROGRESS) {
            struct epoll_event ev;
            ev.events = EPOLLIN | EPOLLOUT;
            ev.data.u32 = counter;
            epoll_ctl(ctx->epoll, EPOLL_CTL_ADD, ctx->peer_socket_array[counter], &ev);
        }

        counter++;
        current_peer = current_peer->next;
    }
    ctx->next_peer_index = counter;
    return true;
}

bool init_download_buffers(const metainfo_t metainfo, torrent_ctx_t *ctx) {
    ctx->bitfield_byte_size = (uint32_t)ceil(metainfo.info->piece_number / 8.0);
    ctx->bitfield = malloc(ctx->bitfield_byte_size);
    if (!ctx->bitfield) return false;
    memset(ctx->bitfield, 0, ctx->bitfield_byte_size);

    ctx->state = init_state(STATE_FILE_LOCATION, metainfo.info->piece_number, metainfo.info->piece_length, ctx->bitfield);

    ctx->block_tracker_bytesize = (uint32_t)ceil(
        ceil(metainfo.info->piece_number * metainfo.info->piece_length / (double)BLOCK_SIZE) / 8.0
    );
    ctx->blocks_per_piece = (uint32_t)ceil(metainfo.info->piece_length / (double)BLOCK_SIZE);
    ctx->block_tracker = malloc(ctx->block_tracker_bytesize);
    if (!ctx->block_tracker) return false;
    memset(ctx->block_tracker, 0, ctx->block_tracker_bytesize);

    ctx->peer_array = malloc(sizeof(peer_t) * ctx->peer_amount);
    if (!ctx->peer_array) return false;
    memset(ctx->peer_array, 0, sizeof(peer_t) * ctx->peer_amount);
    for (uint32_t i = 0; i < ctx->peer_amount; ++i) {
        ctx->peer_array[i].socket = ctx->peer_socket_array[i];
        memset(ctx->peer_array[i].reception_cache, 0, MAX_TRANS_SIZE);
        ctx->peer_array[i].reception_target = 0;
        ctx->peer_array[i].reception_pointer = 0;
        ctx->peer_array[i].am_choking = true;
        ctx->peer_array[i].am_interested = false;
        ctx->peer_array[i].peer_choking = true;
        ctx->peer_array[i].peer_interested = false;
        ctx->peer_array[i].bitfield = nullptr;
        ctx->peer_array[i].status = PEER_NOTHING;
    }
    return true;
}

static void handle_message_payload(torrent_ctx_t *ctx, peer_t *peer, const metainfo_t metainfo, const LOG_CODE log_code) {
    bittorrent_message_t *message = (bittorrent_message_t *) peer->reception_cache;
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
            handle_have(peer, message->payload, ctx->bitfield, ctx->bitfield_byte_size, log_code);
            break;
        case BITFIELD:
            handle_bitfield(peer, message->payload, ctx->bitfield, ctx->bitfield_byte_size, log_code);
            break;
        case REQUEST:
            handle_request(peer, message->payload, log_code);
            break;
        case PIECE: {
            const uint64_t download_size = handle_piece((piece_t*)message->payload, peer->socket, metainfo, ctx->bitfield,
                                                        ctx->block_tracker, ctx->blocks_per_piece, log_code);
            ctx->torrent_stats->downloaded += download_size;
            ctx->torrent_stats->left -= download_size;
            const uint32_t piece_index = ntohl(((piece_t*)message->payload)->index);
            broadcast_have(ctx->peer_array, ctx->peer_amount, piece_index, log_code);
            break;
        }
        case CANCEL:
        case PORT:
            break;
        default: ;
    }

    peer->reception_target = MESSAGE_LENGTH_SIZE;
    peer->reception_pointer = 0;
    memset(peer->reception_cache, 0, ctx->bitfield_byte_size);
    peer->status = PEER_HANDSHAKE_SUCCESS;
}

static void handle_peer_event(torrent_ctx_t *ctx, const struct epoll_event event, const metainfo_t metainfo,
                              const unsigned char *peer_id, const LOG_CODE log_code) {
    const int32_t index = (int32_t)event.data.u32;
    peer_t *peer = &ctx->peer_array[index];

    if (event.events == EPOLLERR) {
        int32_t err = 0;
        socklen_t len = sizeof(err);
        if (getsockopt(peer->socket, SOL_SOCKET, SO_ERROR, &err, &len) < 0) {
            if (log_code >= LOG_ERR) fprintf(stderr, "Getsockopt error %d in socket %d\n", errno, peer->socket);
        } else if (err != 0) {
            errno = err;
            if (log_code >= LOG_ERR) fprintf(stderr, "Socket error %d in socket %d\n", errno, peer->socket);
        }
        epoll_ctl(ctx->epoll, EPOLL_CTL_DEL, peer->socket, nullptr);
        close(peer->socket);
        peer->status = PEER_CLOSED;
        peer->socket = -1;
        return;
    }

    if (peer->status == PEER_NOTHING) {
        peer->status = PEER_CONNECTION_FAILURE;
        if (event.events & EPOLLOUT) {
            int32_t err = 0;
            socklen_t len = sizeof(err);
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
    if (peer->status == PEER_CONNECTION_FAILURE) {
        if (try_connect(peer->socket, &ctx->peer_addr_array[index], log_code)) {
            if (errno != EINPROGRESS) {
                epoll_ctl(ctx->epoll, EPOLL_CTL_DEL, peer->socket, nullptr);
                close(peer->socket);
                peer->status = PEER_CLOSED;
                peer->socket = -1;
            } else {
                peer->status = PEER_NOTHING;
            }
        }
        return;
    }

    read_from_socket(peer, ctx->epoll, log_code);

    if (peer->status == PEER_CONNECTION_SUCCESS && event.events & EPOLLOUT) {
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
            epoll_ctl(ctx->epoll, EPOLL_CTL_DEL, peer->socket, nullptr);
            close(peer->socket);
            peer->status = PEER_CLOSED;
            peer->socket = -1;
        }
        return;
    }

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
            epoll_ctl(ctx->epoll, EPOLL_CTL_DEL, peer->socket, nullptr);
            close(peer->socket);
            peer->status = PEER_CLOSED;
            peer->socket = -1;
        }
        memset(peer->reception_cache, 0, MAX_TRANS_SIZE);
    }

    if ((peer->status == PEER_HANDSHAKE_SUCCESS || peer->status == PEER_BITFIELD_RECEIVED) && event.events & EPOLLOUT) {
        char *buffer = malloc(MESSAGE_LENGTH_AND_ID_SIZE + ctx->bitfield_byte_size);
        if (buffer) {
            uint32_t length = 1 + ctx->bitfield_byte_size;
            length = htonl(length);
            memcpy(buffer, &length, MESSAGE_LENGTH_SIZE);
            buffer[MESSAGE_LENGTH_SIZE] = BITFIELD;
            memcpy(buffer + 5, ctx->bitfield, ctx->bitfield_byte_size);
            int64_t sent_bytes = 0;
            while (sent_bytes < MESSAGE_LENGTH_AND_ID_SIZE + ctx->bitfield_byte_size) {
                int64_t sent = send(peer->socket, buffer + sent_bytes,
                                    MESSAGE_LENGTH_AND_ID_SIZE + ctx->bitfield_byte_size - sent_bytes, 0);
                if (sent > 0) sent_bytes += sent;
            }
            free(buffer);
        }
    }

    if (peer->status >= PEER_HANDSHAKE_SUCCESS && peer->reception_target == peer->reception_pointer &&
        peer->reception_target == MESSAGE_LENGTH_SIZE) {
        if (read_message_length(peer->reception_cache, &peer->last_msg)) {
            peer->reception_target = MESSAGE_LENGTH_AND_ID_SIZE;
            peer->status = PEER_AWAITING_ID;
        } else {
            peer->reception_target = MESSAGE_LENGTH_SIZE;
            peer->reception_pointer = 0;
        }
        if (log_code == LOG_FULL) fprintf(stdout, "Peer %d received length\n", peer->socket);
    }

    if (peer->status >= PEER_AWAITING_ID && peer->reception_target == peer->reception_pointer &&
        peer->reception_target == MESSAGE_LENGTH_AND_ID_SIZE) {
        bittorrent_message_t *message = (bittorrent_message_t *) peer->reception_cache;
        if (message->length > 1) {
            peer->reception_target += (int32_t) message->length - 1;
            peer->status = PEER_AWAITING_PAYLOAD;
        } else {
            peer->reception_target = MESSAGE_LENGTH_SIZE;
            peer->reception_pointer = 0;
        }
        if (log_code == LOG_FULL) fprintf(stdout, "Peer %d received id of %d with length of %d\n", peer->socket,
                                          message->id, message->length);
    }

    if (peer->status >= PEER_AWAITING_PAYLOAD && peer->reception_target == peer->reception_pointer) {
        handle_message_payload(ctx, peer, metainfo, log_code);
    }
}

int32_t run_main_peer_loop(torrent_ctx_t *ctx, const metainfo_t metainfo, const unsigned char *peer_id,
                           const LOG_CODE log_code) {
    struct epoll_event epoll_events[MAX_EVENTS];

    while (ctx->torrent_stats->left > 0) {
        const int32_t nfds = epoll_wait(ctx->epoll, epoll_events, MAX_EVENTS, EPOLL_TIMEOUT);
        if (nfds == -1) {
            if (log_code >= LOG_ERR) fprintf(stderr, "Error in epoll_wait\n");
            continue;
        }
        if (nfds == 0) {
            if (log_code >= LOG_ERR) fprintf(stderr, "Epoll timeout\n");
            continue;
        }

        for (int32_t i = 0; i < nfds; ++i) {
            handle_peer_event(ctx, epoll_events[i], metainfo, peer_id, log_code);
        }

        write_state(STATE_FILE_LOCATION, ctx->state);
        for (uint32_t i = 0; i < ctx->peer_amount; ++i) {
            if (ctx->peer_array[i].status == PEER_CLOSED && difftime(time(nullptr), ctx->peer_array[i].last_msg) >= 10) {
                fprintf(stdout, "Attempting to reconnect socket #%d\n", ctx->peer_array[i].socket);
                reconnect(ctx->peer_array, ctx->peer_amount, (uint32_t)ctx->next_peer_index, ctx->epoll, log_code);
            }
        }
    }

    return 0;
}

int32_t torrent(const metainfo_t metainfo, const unsigned char *peer_id, const LOG_CODE log_code) {
    torrent_ctx_t ctx = {0};
    ctx.epoll = -1;
    int32_t result = -1;

    if (!init_torrent_stats_and_tracker(metainfo, peer_id, log_code, &ctx)) {
        cleanup_torrent_ctx(&ctx);
        return -1;
    }
    if (!init_peer_connections(&ctx, log_code)) {
        cleanup_torrent_ctx(&ctx);
        return -1;
    }
    if (!init_download_buffers(metainfo, &ctx)) {
        cleanup_torrent_ctx(&ctx);
        return -1;
    }

    result = run_main_peer_loop(&ctx, metainfo, peer_id, log_code);
    cleanup_torrent_ctx(&ctx);
    return result;
}
