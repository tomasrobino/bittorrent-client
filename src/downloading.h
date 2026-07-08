#ifndef DOWNLOADING_H
#define DOWNLOADING_H

#include <netinet/in.h>

#include "downloading_types.h"
#include "file.h"
#include "predownload_udp.h"

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
 * @param torrent_stats Pointer to structure containing torrent statistics including
 *                      downloaded bytes, bytes left, uploaded bytes, event type, and key
 * @param log_code Controls the verbosity of logging output. Can be LOG_NO (no logging),
 *                 LOG_ERR (error logging), LOG_SUMM (summary logging), or
 *                 LOG_FULL (detailed logging).
 * @return A pointer to an announce_response_t structure containing the tracker's response, or nullptr if the request fails.
 */
announce_response_t* handle_predownload_udp(metainfo_t metainfo, const unsigned char *peer_id, const torrent_stats_t* torrent_stats, LOG_CODE log_code);


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
 * @param epoll Epoll instance
 * @param log_code Specifies the level of logging. Acceptable values are
 *                 LOG_NO (no logging), LOG_ERR (log errors), LOG_SUMM (log summary),
 *                 or LOG_FULL (full logging).
 * @return True if data was successfully read or no errors occurred that
 *         require terminating the connection. Returns false if an unrecoverable
 *         error occurs or if the remote peer has closed the connection.
 */
bool read_from_socket(peer_t* peer, int32_t epoll, LOG_CODE log_code);

/**
 * Attempts to reconnect to peers in the provided peer list that are marked with a status of PEER_CLOSED.
 * For each peer marked as PEER_CLOSED, the function attempts to reset its state, create a new non-blocking
 * socket, and initiate a connection. If a peer's connection is in progress, its socket is added to the
 * epoll instance for monitoring future events.
 *
 * @param peer_list Pointer to an array of peers to reconnect.
 * @param peer_amount The total number of peers in the peer list.
 * @param last_peer The current count of handled peers, used to assign unique identifiers to new connections.
 * @param epoll Epoll instance
 * @param log_code Controls the verbosity of error logging. Supported values are determined by the LOG_CODE enum.
 *                 For example, LOG_ERR will log errors during the connection process.
 * @return The updated value of last_peer, incremented for each successfully reset peer.
 */
uint32_t reconnect(peer_t* peer_list, uint32_t peer_amount, uint32_t last_peer, int32_t epoll, LOG_CODE log_code);

/**
 * Runtime context used by the torrent download pipeline.
 * It owns all long-lived resources needed by `torrent()`, including sockets,
 * peer state, trackers, download bitmaps, and persisted download state.
 */
typedef struct {
    torrent_stats_t *torrent_stats;
    announce_response_t *announce_response;
    uint32_t peer_amount;
    int32_t *peer_socket_array;
    struct sockaddr_in *peer_addr_array;
    int32_t epoll;
    int32_t next_peer_index;
    unsigned char *bitfield;
    uint32_t bitfield_byte_size;
    state_t *state;
    unsigned char *block_tracker;
    uint32_t block_tracker_bytesize;
    uint32_t blocks_per_piece;
    peer_t *peer_array;
} torrent_ctx_t;

/**
 * Counts the number of peers returned by the tracker announce response.
 *
 * @param announce_response Tracker announce response containing the linked peer list.
 * @return Total amount of peers in the response.
 */
uint32_t count_peers(const announce_response_t *announce_response);

/**
 * Frees a tracker announce response and its linked peer list.
 *
 * @param announce_response Response object to release. Null is allowed.
 */
void free_announce_response(announce_response_t *announce_response);

/**
 * Releases all resources stored in a torrent runtime context.
 * This includes socket descriptors, dynamic buffers, peer arrays, tracker data,
 * and torrent statistics.
 *
 * @param ctx Context to clean. Null is allowed.
 */
void cleanup_torrent_ctx(torrent_ctx_t *ctx);

/**
 * Initializes torrent statistics and performs tracker predownload/announce.
 *
 * @param metainfo Torrent metadata parsed from the .torrent file.
 * @param peer_id Local peer id used in tracker requests.
 * @param log_code Logging level.
 * @param ctx Context to initialize.
 * @return true on success, false if allocation or tracker communication fails.
 */
bool init_torrent_stats_and_tracker(metainfo_t metainfo, const unsigned char *peer_id, LOG_CODE log_code, torrent_ctx_t *ctx);

/**
 * Initializes peer sockets, addresses, and epoll registration using tracker peers.
 *
 * @param ctx Initialized context containing announce response.
 * @param log_code Logging level.
 * @return true on success, false on allocation/socket/init failures.
 */
bool init_peer_connections(torrent_ctx_t *ctx, LOG_CODE log_code);

/**
 * Initializes in-memory download tracking data:
 * - client bitfield
 * - block tracker
 * - per-peer runtime structures
 * - persisted state abstraction
 *
 * @param metainfo Torrent metadata.
 * @param ctx Context to initialize.
 * @return true on success, false on allocation/init failures.
 */
bool init_download_buffers(metainfo_t metainfo, torrent_ctx_t *ctx);

/**
 * Runs the main epoll-driven peer interaction loop until download completion.
 *
 * @param ctx Fully initialized torrent runtime context.
 * @param metainfo Torrent metadata.
 * @param peer_id Local peer id.
 * @param log_code Logging level.
 * @return 0 on success, non-zero on failure.
 */
int32_t run_main_peer_loop(torrent_ctx_t *ctx, metainfo_t metainfo, const unsigned char *peer_id, LOG_CODE log_code);

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
