#ifndef DOWNLOADING_H
#define DOWNLOADING_H
#include "structs.h"

// Maximum events cached by epoll
#define MAX_EVENTS 128
// Maximum amount of time epoll will wait for sockets to be ready (in milliseconds)
#define EPOLL_TIMEOUT 5000
// Enum for peer statuses
typedef enum {
    PEER_NOTHING, // Untouched peer
    PEER_CONNECTED, // Peer connected
    PEER_NO_CONNECTION, // Peer failed connection

} PEER_STATUS;

/**
 * Attempts to establish a connection to a peer using the specified socket and address.
 * @param sockfd The file descriptor of the socket to be used for the connection.
 * @param peer_addr Pointer to a sockaddr_in structure containing the peer's address.
 * @return Returns 1 if the connection is successfully initiated or is in progress;
 *         returns 0 and closes the socket if the connection fails with an error
 *         other than EINPROGRESS.
 */
int try_connect(int sockfd, const struct sockaddr_in* peer_addr);
/**
 * Attempts to get a handshake with a peer to download the torrent passed in the info_hash
 * @param server_addr Server address
 * @param sockfd Socket file descriptor
 * @param info_hash The torrent's info hash
 * @param peer_id This client's peer_id
 * @returns The 20-byte peer_id of the peer (non null-terminated); or nullptr if unsuccessful
 */
char* handshake(const struct sockaddr *server_addr, int sockfd, const char* info_hash, const char* peer_id);
/**
 * Downloads & uploads torrent
 * @param metainfo The torrent metainfo extracted from the .torrent file
 * @param peer_id The chosen peer_id
 * @returns 0 for success, !0 for failure
*/
int torrent(metainfo_t metainfo, const char* peer_id);
#endif //DOWNLOADING_H
