//
// Created by Tomas on 12/11/2025.
//

#ifndef BITTORRENT_CLIENT_DOWNLOADING_TYPES_H
#define BITTORRENT_CLIENT_DOWNLOADING_TYPES_H

#include <sys/time.h>

/// @brief Maximum events cached by epoll
#define MAX_EVENTS 128
/// @brief Maximum amount of time epoll will wait for sockets to be ready (in milliseconds)
#define EPOLL_TIMEOUT 5000
/// @brief Download block size in bytes (16KB)
#define BLOCK_SIZE 16384
/// @brief Maximum amount of bytes to be transmited in any request or response
#define MAX_TRANS_SIZE (BLOCK_SIZE+9)
// TODO Temporal solution, this should be changed to be adjusted dynamically

/// @brief Amount of block requests to queue for each peer
#define QUEUE_SIZE 5

/// @brief Size of state_t minus padding, and bitfield pointer
#define STATE_T_CORE_SIZE 13

/// @brief Enum for peer statuses
typedef enum {
    PEER_CLOSED, /** This peer's socket has been closed */
    PEER_NOTHING, /**< Untouched peer */
    PEER_CONNECTION_SUCCESS, /**< Peer connected */
    PEER_CONNECTION_FAILURE, /**< Peer failed connection */
    PEER_HANDSHAKE_SENT, /**< Handshake sent to peer */
    PEER_HANDSHAKE_SUCCESS, /**< Handshake completed successfully */
    PEER_AWAITING_ID, /**< Already received message length, so now waiting for id */
    PEER_AWAITING_PAYLOAD, /**< Already received message length and id, so now waiting for the payload */
    PEER_BITFIELD_RECEIVED, /**< Received bitfield from peer */
} PEER_STATUS;

/// @brief Represents the persistent download state for a torrent
typedef struct {
    uint32_t magic; /**< Magic number to identify valid state files */
    uint8_t version; /**< State file format version */
    uint32_t piece_count; /**< Total number of pieces in the torrent */
    uint32_t piece_size; /**< Size of each piece in bytes */
    unsigned char* bitfield; /**< Bit array representing which pieces have been downloaded */
} state_t;


typedef struct {
    uint32_t downloaded;
    uint32_t left;
    uint32_t uploaded;
    uint32_t event;
    uint32_t key;
} torrent_stats_t;

/// @brief Represents peer data and state in a BitTorrent connection
typedef struct {
    int socket; /**< Socket file descriptor for this peer connection */
    int reception_target; /**< The amount of bytes this peer is expecting to receive */
    int reception_pointer; /**< How many bytes were already red into reception_cache for this reception_target */
    bool am_choking; /**< Whether we are choking the peer */
    bool am_interested; /**< Whether we are interested in peer's pieces */
    bool peer_choking; /**< Whether we are choked the peer */
    bool peer_interested; /**< Whether peer is interested in our pieces */
    unsigned char *bitfield; /**< Bit array representing the pieces this peer has */
    unsigned char *id; /**< 20-byte string peer ID used during handshake */
    unsigned char reception_cache[MAX_TRANS_SIZE]; /**< Cache for storing read bytes before interpreting them
                                                    * TODO Maybe make this dynamic, to save on RAM
                                                    */
    PEER_STATUS status; /**< Current status of the peer connection */
    time_t last_msg; /**< Timestamp of last message received from peer */
    struct sockaddr_in* address;
} peer_t;

#endif //BITTORRENT_CLIENT_DOWNLOADING_TYPES_H