#ifndef DOWNLOADING_H
#define DOWNLOADING_H
#include "file.h"
/// @brief Maximum events cached by epoll
#define MAX_EVENTS 128
/// @brief Maximum amount of time epoll will wait for sockets to be ready (in milliseconds)
#define EPOLL_TIMEOUT 5000
/// @brief Download block size in bytes (16KB)
#define BLOCK_SIZE 16384

/// @brief Enum for peer statuses
typedef enum {
    PEER_NOTHING, /**< Untouched peer */
    PEER_CONNECTION_SUCCESS, /**< Peer connected */
    PEER_CONNECTION_FAILURE, /**< Peer failed connection */
    PEER_HANDSHAKE_SENT, /**< Handshake sent to peer */
    PEER_HANDSHAKE_SUCCESS, /**< Handshake completed successfully */
    PEER_BITFIELD_RECEIVED, /**< Received bitfield from peer */
    PEER_NO_BITFIELD, /**< No bitfield received from peer */
    PEER_PENDING_HAVE /**< Need to send have message to all peers */
} PEER_STATUS;

/// @brief Represents peer data and state in a BitTorrent connection
typedef struct {
    unsigned char *bitfield; /**< Bit array representing the pieces this peer has */
    char *id; /**< 20-byte string peer ID used during handshake */
    PEER_STATUS status; /**< Current status of the peer connection */
    bool peer_choked; /**< Whether we are choking by the peer */
    bool client_choked; /**< Whether we are choked the peer */
    bool peer_interest; /**< Whether peer is interested in our pieces */
    bool client_interest; /**< Whether we are interested in peer's pieces */
    int socket; /**< Socket file descriptor for this peer connection */
    time_t last_msg; /**< Timestamp of last message received from peer */
} peer_t;

/**
 * Calculates the size of a block to be downloaded based on the piece size and byte offset.
 * The block size is typically a fixed value (BLOCK_SIZE), except for the last block in the piece,
 * which may be smaller depending on the remaining size of the piece.
 *
 * @param piece_size The total size of the piece in bytes.
 * @param byte_offset The byte offset within the piece from where the block starts.
 * @return The size of the block in bytes to be downloaded. If the block is the last one in the piece,
 *         its size will be smaller than or equal to BLOCK_SIZE. Otherwise, it will always return BLOCK_SIZE.
 */
int64_t calc_block_size(unsigned int piece_size, unsigned int byte_offset);
/**
 * @brief Constructs a full file path as a string from a linked list of directory segments
 * and creates the necessary directory structure if it does not exist.
 *
 * @param filepath A pointer to a linked list of `ll` structures, where each node
 * represents a segment of the file path as a string.
 * @return A dynamically allocated string containing the full file path. The caller
 * is responsible for freeing the allocated memory.
 */
char *get_path(const ll *filepath);

/**
 * @brief Reads a block of data from a specified socket and writes it into the provided buffer.
 *
 * This function continuously reads data from the given socket until the requested
 * amount of data is read or an error occurs.
 *
 * @param sockfd The socket file descriptor from which data is to be read.
 * @param buffer A pointer to the buffer where the read data will be stored.
 * @param amount The amount of data, in bytes, to read from the socket.
 * @return The total number of bytes successfully read from the socket. Returns -1 in case of an error.
 */
int32_t read_block_from_socket(int sockfd, unsigned char *buffer, int64_t amount);

/**
 * @brief Writes a specified number of bytes from a buffer to a given file.
 *
 * @param buffer Pointer to the buffer containing the data to be written.
 * @param amount Number of bytes to write to the file.
 * @param file Pointer to the file object where data will be written.
 * @return The number of bytes successfully written, or -1 if an error occurred.
 */
int32_t write_block(const unsigned char *buffer, int64_t amount, FILE *file);

/**
 * @brief Downloads a specific block of data from a peer and writes it to the corresponding file(s).
 *
 * @param sockfd The socket file descriptor used for communication with the peer.
 * @param piece_index The index of the piece to which the block belongs.
 * @param piece_size The size of the piece in bytes.
 * @param byte_offset The offset within the piece where the block begins.
 * @param files_metainfo A linked list containing metadata about the files managed by the torrent client,
 *                       including their lengths and paths. The files must be in order according to their index
 * @return Returns 0 on successful downloading and writing of the block. An error code may otherwise be returned.
 */
int download_block(int sockfd, unsigned int piece_index, unsigned int piece_size, unsigned int byte_offset,
                   files_ll *files_metainfo);

/**
 * Determines if a specific piece of a torrent has been fully downloaded.
 *
 * This function checks the `block_tracker` bitfield to verify if all blocks
 * within the given piece are marked as downloaded. The size of the piece
 * is adjusted if it is the last piece and does not span the full `piece_size`.
 *
 * @param block_tracker A pointer to the bitfield that tracks the downloaded state
 *                      of each block in the torrent.
 * @param piece_index The index of the piece to check.
 * @param piece_size The standard size of a piece in bytes.
 * @param torrent_size The total size of the torrent in bytes.
 *
 * @return Returns `true` if the piece at the given index is fully downloaded,
 *         otherwise returns `false`.
 */
bool piece_complete(const unsigned char *block_tracker, unsigned int piece_index, unsigned int piece_size, int64_t torrent_size);
/**
 * @brief Downloads & uploads torrent
 * @param metainfo The torrent metainfo extracted from the .torrent file
 * @param peer_id The chosen peer_id
 * @return 0 for success, !0 for failure
 */
int torrent(metainfo_t metainfo, const char *peer_id);
#endif //DOWNLOADING_H
