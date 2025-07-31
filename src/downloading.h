#ifndef DOWNLOADING_H
#define DOWNLOADING_H
#include "structs.h"

// Maximum events cached by epoll
#define MAX_EVENTS 128
// Maximum amount of time epoll will wait for sockets to be ready (in milliseconds)
#define EPOLL_TIMEOUT 5000
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
