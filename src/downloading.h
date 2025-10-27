#ifndef DOWNLOADING_H
#define DOWNLOADING_H
#include "file.h"
#include "predownload_udp.h"

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
int64_t calc_block_size(uint32_t piece_size, uint32_t byte_offset);
/**
 * @brief Constructs a full file path as a string from a linked list of directory segments
 * and creates the necessary directory structure if it does not exist.
 *
 * @param filepath A pointer to a linked list of `ll` structures, where each node
 * represents a segment of the file path as a string.
 * @param log_code Controls the verbosity of logging output. Can be LOG_NO (no logging),
 *                 LOG_ERR (error logging), LOG_SUMM (summary logging), or 
 *                 LOG_FULL (detailed logging).
 * @return A dynamically allocated string containing the full file path. The caller
 * is responsible for freeing the allocated memory.
 */
char *get_path(const ll *filepath, LOG_CODE log_code);

/**
 * @brief Writes a specified number of bytes from a buffer to a given file.
 *
 * @param buffer Pointer to the buffer containing the data to be written.
 * @param amount Number of bytes to write to the file.
 * @param file Pointer to the file object where data will be written.
 * @param log_code Controls the verbosity of logging output. Can be LOG_NO (no logging),
 *                 LOG_ERR (error logging), LOG_SUMM (summary logging), or
 *                 LOG_FULL (detailed logging).
 * @return The number of bytes successfully written, or -1 if an error occurred.
 */
int32_t write_block(const unsigned char *buffer, int64_t amount, FILE *file, LOG_CODE log_code);
/**
 * Processes a block of data downloaded from a peer. The function determines the piece and offset
 * from the provided buffer, validates the input parameters, and processes the data within the
 * linked list of file metadata.
 *
 * @param buffer Pointer to the received buffer containing the block data as well as piece index
 *               and byte offset in network byte order.
 * @param piece_size The size of a single piece in bytes. This value is used to validate the offset.
 * @param files_metainfo Pointer to the linked list of file metadata containing information
 *                       about the files in the torrent and their respective byte ranges.
 * @param log_code Logging level indicating the verbosity of the logging for debugging and error reporting.
 *
 * @return An integer status code:
 *         - 0: Block processed successfully.
 *         - 1: Invalid arguments (e.g., offset greater than piece size or piece size is 0).
 *         - 2: Failed to open file.
 *         - 3: Write error.
 */
int32_t process_block(const unsigned char *buffer, uint32_t piece_size,
                      files_ll *files_metainfo, LOG_CODE log_code);

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
bool piece_complete(const unsigned char *block_tracker, uint32_t piece_index, uint32_t piece_size, int64_t torrent_size);

/**
 * Checks if all bits in the specified range are set within a given bitfield.
 *
 * This function examines the specified range of bits, defined by `start` and `end`,
 * located within the provided `bitfield`. It verifies if all the bits in that range
 * are set to 1.
 *
 * @param bitfield A pointer to the bitfield array that contains the bits to be checked.
 * @param start The starting bit index of the range to check (inclusive).
 * @param end The ending bit index of the range to check (inclusive).
 * @return Returns true if all bits in the range are set to 1, otherwise returns false.
 */
bool are_bits_set(const unsigned char *bitfield, uint32_t start, uint32_t end);

/**
 * Closes files in a linked list if all pieces overlapping with the file are downloaded.
 *
 * This function checks for pieces of a file that intersect with the given
 * piece and determines whether all corresponding pieces are downloaded or not.
 * If all overlapping pieces for a file are downloaded, the file pointer is closed.
 *
 * @param files Pointer to the linked list of files (each file containing metadata and a file pointer).
 * @param bitfield A bitfield indicating which pieces are downloaded (1 indicates downloaded, 0 indicates not).
 * @param piece_index The index of the piece to be processed.
 * @param piece_size The size of a piece in bytes.
 * @param this_piece_size The size of the current piece being evaluated (useful for the last piece which can be smaller).
 */
void closing_files(const files_ll* files, const unsigned char* bitfield, uint32_t piece_index, uint32_t piece_size, uint32_t
                   this_piece_size);

/**
 * Handles the pre-download procedure over UDP by connecting to a tracker and sending an announce request.
 *
 * @param metainfo A structure containing metadata of the torrent file, including tracker information.
 * @param peer_id A unique identifier for the peer that is making the request.
 * @param downloaded The total amount of data downloaded by the client, in bytes.
 * @param left The amount of data left to download, in bytes.
 * @param uploaded The total amount of data uploaded by the client, in bytes.
 * @param event A numeric value representing the event type (e.g., start, stop, complete).
 * @param key A random numerical key used to verify the announce request.
 * @param log_code Controls the verbosity of logging output. Can be LOG_NO (no logging),
 *                 LOG_ERR (error logging), LOG_SUMM (summary logging), or
 *                 LOG_FULL (detailed logging).
 * @return A pointer to an announce_response_t structure containing the tracker's response, or nullptr if the request fails.
 */
announce_response_t* handle_predownload_udp(metainfo_t metainfo, const unsigned char *peer_id, uint64_t downloaded, uint64_t left, uint64_t uploaded, uint32_t event, uint32_t key, LOG_CODE log_code);


/**
 * Reads available data from a peer's socket into the reception cache.
 *
 * This method attempts to read data from the peer's socket until the
 * specified `reception_target` is reached or until an error occurs
 * (non-blocking errors excluded). In case the peer has closed the
 * connection, the socket is shut down and closed, and the peer's
 * status is updated accordingly.
 *
 * @param peer A pointer to the peer_t structure representing the peer
 *             whose socket is to be read from. Contains state information
 *             for the peer, including the reception cache and pointers.
 * @param log_code Specifies the level of logging. Acceptable values are
 *                 LOG_NO (no logging), LOG_ERR (log errors), LOG_SUMM (log summary),
 *                 or LOG_FULL (full logging).
 * @return true if data was successfully read or no errors occurred that
 *         require terminating the connection. Returns false if an unrecoverable
 *         error occurs or if the remote peer has closed the connection.
 */
bool read_from_socket(peer_t* peer, LOG_CODE log_code);

/**
 * Attempts to reconnect to peers in the provided peer list that are marked with a status of PEER_CLOSED.
 * For each peer marked as PEER_CLOSED, the function attempts to reset its state, create a new non-blocking
 * socket, and initiate a connection. If a peer's connection is in progress, its socket is added to the
 * epoll instance for monitoring future events.
 *
 * @param peer_list Pointer to an array of peers to reconnect.
 * @param peer_amount The total number of peers in the peer list.
 * @param last_peer The current count of handled peers, used to assign unique identifiers to new connections.
 * @param log_code Controls the verbosity of error logging. Supported values are determined by the LOG_CODE enum.
 *                 For example, LOG_ERR will log errors during the connection process.
 * @return The updated value of last_peer, incremented for each successfully reset peer.
 */
uint32_t reconnect(peer_t* peer_list, uint32_t peer_amount, uint32_t last_peer, LOG_CODE log_code);

/**
 * @brief Downloads & uploads torrent
 * @param metainfo The torrent metainfo extracted from the .torrent file
 * @param peer_id The chosen peer_id
 * @param log_code An enumeration value specifying the desired logging level.
 *                    It can be one of the following:
 *                    LOG_NO (no logging), LOG_ERR (error logging),
 *                    LOG_SUMM (summary logging), or LOG_FULL (detailed logging).
 * @return 0 for success, !0 for failure
 */
int32_t torrent(metainfo_t metainfo, const unsigned char *peer_id, LOG_CODE log_code);
#endif //DOWNLOADING_H
