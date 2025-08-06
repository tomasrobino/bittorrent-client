#ifndef DOWNLOADING_H
#define DOWNLOADING_H

// Maximum events cached by epoll
#define MAX_EVENTS 128
// Maximum amount of time epoll will wait for sockets to be ready (in milliseconds)
#define EPOLL_TIMEOUT 5000
#include "file.h"

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

typedef struct {
    char* bitfield;
    char* id;
    PEER_STATUS status;
    int socket;
} peer_t;

/**
 * Downloads & uploads torrent
 * @param metainfo The torrent metainfo extracted from the .torrent file
 * @param peer_id The chosen peer_id
 * @returns 0 for success, !0 for failure
*/
int torrent(metainfo_t metainfo, const char* peer_id);
#endif //DOWNLOADING_H
