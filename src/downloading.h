#ifndef DOWNLOADING_H
#define DOWNLOADING_H
#include "file.h"
// Maximum events cached by epoll
#define MAX_EVENTS 128
// Maximum amount of time epoll will wait for sockets to be ready (in milliseconds)
#define EPOLL_TIMEOUT 5000
// Download block size in bytes (16KB)
#define BLOCK_SIZE 16384

// Enum for peer statuses
typedef enum {
    PEER_NOTHING, // Untouched peer
    PEER_CONNECTION_SUCCESS, // Peer connected
    PEER_CONNECTION_FAILURE, // Peer failed connection
    PEER_HANDSHAKE_SENT,
    PEER_HANDSHAKE_SUCCESS,
    PEER_BITFIELD_RECEIVED,
    PEER_NO_BITFIELD
} PEER_STATUS;

/**
 * Represents peer data and state in a BitTorrent connection.
 *
 * @field bitfield Bit array representing the pieces this peer has
 * @field id 20-byte string peer ID used during handshake 
 * @field status Current status of the peer connection 
 * @field peer_choked Whether we are choking by the peer
 * @field client_choked Whether we are choked the peer
 * @field peer_interest Whether peer is interested in our pieces
 * @field client_interest Whether we are interested in peer's pieces
 * @field socket Socket file descriptor for this peer connection
 * @field last_msg Timestamp of last message received from peer
 */
typedef struct {
    unsigned char *bitfield;
    char *id;
    PEER_STATUS status;
    bool peer_choked;
    bool client_choked;
    bool peer_interest;
    bool client_interest;
    int socket;
    time_t last_msg;
} peer_t;

int download_block(const int sockfd, const unsigned int piece_index, const unsigned int piece_size, const unsigned int block_index, const unsigned int blocks_per_piece, const files_ll* files_metainfo)

/**
 * Downloads & uploads torrent
 * @param metainfo The torrent metainfo extracted from the .torrent file
 * @param peer_id The chosen peer_id
 * @returns 0 for success, !0 for failure
*/
int torrent(metainfo_t metainfo, const char* peer_id);
#endif //DOWNLOADING_H
