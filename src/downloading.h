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
    PEER_NO_BITFIELD,
    PEER_PENDING_WRITE
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

/**
 * Constructs a full file path as a string from a linked list of directory segments
 * and creates the necessary directory structure if it does not exist.
 *
 * @param filepath A pointer to a linked list of `ll` structures, where each node
 * represents a segment of the file path as a string.
 * @return A dynamically allocated string containing the full file path. The caller
 * is responsible for freeing the allocated memory.
 */
char* get_path(const ll* filepath);

/**
 * Reads a block of data from a specified socket and writes it into the provided buffer.
 *
 * This function continuously reads data from the given socket until the requested
 * amount of data is read or an error occurs.
 *
 * @param sockfd The socket file descriptor from which data is to be read.
 * @param buffer A pointer to the buffer where the read data will be stored.
 * @param amount The amount of data, in bytes, to read from the socket.
 * @return The total number of bytes successfully read from the socket. Returns -1 in case of an error.
 */
int32_t read_block_from_socket(int sockfd, unsigned char* buffer, int32_t amount);
/**
 * Writes a specified number of bytes from a buffer to a given file.
 *
 * @param buffer Pointer to the buffer containing the data to be written.
 * @param amount Number of bytes to write to the file.
 * @param file Pointer to the file object where data will be written.
 * @return The number of bytes successfully written, or -1 if an error occurred.
 */
int32_t write_block(const unsigned char* buffer, int32_t amount, FILE* file);
/**
 * Downloads a specific block of data from a peer and writes it to the corresponding file(s) based on the metadata.
 *
 * @param sockfd The socket file descriptor used for communication with the peer.
 * @param piece_index The index of the piece to which the block belongs.
 * @param piece_size The size of the piece in bytes.
 * @param byte_offset The offset within the piece where the block begins.
 * @param files_metainfo A linked list containing metadata about the files managed by the torrent client,
 *                       including their lengths and paths.
 * @return Returns 0 on successful downloading and writing of the block. An error code may otherwise be returned.
 */
int download_block(int sockfd, unsigned int piece_index, unsigned int piece_size, unsigned int byte_offset, files_ll* files_metainfo);

/**
 * Downloads & uploads torrent
 * @param metainfo The torrent metainfo extracted from the .torrent file
 * @param peer_id The chosen peer_id
 * @returns 0 for success, !0 for failure
*/
int torrent(metainfo_t metainfo, const char* peer_id);
#endif //DOWNLOADING_H
