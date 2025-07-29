#ifndef DOWNLOADING_H
#define DOWNLOADING_H
#include "structs.h"

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
 * To be called when creating a thread. Handles interaction with a specific peer.
 * @param passed_args Arguments for functions used when interacting with the peer
 * @returns Exit code
 */
void* thread_peer(void* passed_args);
/**
 * Downloads & uploads torrent
 * @param metainfo The torrent metainfo extracted from the .torrent file
 * @param peer_id The chosen peer_id
 * @returns 0 for success, !0 for failure
*/
int torrent(metainfo_t metainfo, const char* peer_id);
#endif //DOWNLOADING_H
