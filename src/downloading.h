#ifndef DOWNLOADING_H
#define DOWNLOADING_H
#include <stdint.h>

#include "structs.h"

/**
 * Sends an announce request to the tracker
 * @param server_addr Server address
 * @param sockfd Socket file descriptor
 * @param connection_id Returned from connect response of the tracker
 * @param info_hash Info hash of the torrent
 * @param peer_id ID of this peer
 * @param downloaded Amount downloaded
 * @param left Amount left to download
 * @param uploaded Amount uploaded
 * @param event
 * @param key Torrent key
 * @param port Tracker port
 * @return The announce response from the server or nullptr if an error ocurred
 */
announce_response_t* announce_request_udp(
    const struct sockaddr *server_addr,
    int sockfd,
    uint64_t connection_id,
    const char info_hash[],
    const char peer_id[],
    uint64_t downloaded,
    uint64_t left,
    uint64_t uploaded,
    uint32_t event,
    uint32_t key,
    uint16_t port
);

/**
 * Requests torrent scrape data to tracker
 * @param server_addr Server address
 * @param sockfd Socket file descriptor
 * @param connection_id Returned from connect response of the tracker
 * @param info_hash Info hash of all the torrents
 * @param torrent_amount The amount of torrents to ask about
 * @return The data returned or nullptr if there's an error
 */
scrape_response_t* scrape_request_udp(const struct sockaddr *server_addr, int sockfd, uint64_t connection_id, const char info_hash[], unsigned int torrent_amount);

/**
 * Attempts to get a handshake with a peer to download the torrent passed in the info_hash
 * @param server_addr Server address
 * @param sockfd Socket file descriptor
 * @param info_hash Info hash the torrent
 * @param peer_id This client's peer_id
 * @returns The 20-byte peer_id of the peer; or nullptr if unsuccessful
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
