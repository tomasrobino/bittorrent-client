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
    PEER_CONNECTION_SUCCESS, // Peer connected
    PEER_CONNECTION_FAILURE, // Peer failed connection
    PEER_HANDSHAKE_SENT,
    PEER_HANDSHAKE_SUCCESS
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
 * Sends a BitTorrent handshake message through the specified socket.
 * A handshake is required to identify and establish communication with the peer.
 *
 * @param sockfd The file descriptor of the socket used to communicate with the peer.
 * @param info_hash A 20-byte string representing the SHA1 hash of the torrent's info dictionary.
 * @param peer_id A 20-byte unique identifier for the peer initiating the handshake.
 * @return Returns the number of bytes successfully sent (68 bytes for a complete handshake);
 *         returns a negative value if an error occurs while sending the handshake.
 */
int send_handshake(int sockfd, const char* info_hash, const char* peer_id);

/**
 * Processes the response during a handshake interaction by validating the
 * info hash and extracting the peer's ID.
 * @param sockfd The file descriptor of the socket from which the response is received.
 * @param info_hash A pointer to the 20-byte info hash that is expected to match the
 *                  handshake response.
 * @return Returns a dynamically allocated 20-byte character array containing the
 *         peer's ID if the handshake is successful; returns nullptr if the validation
 *         fails or an error occurs.
 */
char* handshake_response(int sockfd, const char* info_hash);
/**
 * Downloads & uploads torrent
 * @param metainfo The torrent metainfo extracted from the .torrent file
 * @param peer_id The chosen peer_id
 * @returns 0 for success, !0 for failure
*/
int torrent(metainfo_t metainfo, const char* peer_id);
#endif //DOWNLOADING_H
