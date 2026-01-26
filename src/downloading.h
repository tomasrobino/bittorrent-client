#ifndef DOWNLOADING_H
#define DOWNLOADING_H

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
announce_response_t* handle_predownload_udp(metainfo_t metainfo, const unsigned char *peer_id, torrent_stats_t* torrent_stats, LOG_CODE log_code);


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
 * @return true if data was successfully read or no errors occurred that
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
 * @brief Writes and serializes the torrent download state to a file.
 *
 * This function saves the current torrent state to the specified file,
 * including information about which pieces have been downloaded. The state
 * is stored in a binary format containing a magic number, version, piece count,
 * piece size, and a bitfield representing downloaded pieces.
 *
 * @param filename The path to the file where the serialized download state will be written.
 * @param state A pointer to the state_t structure containing the download state to be serialized.
 * @return 0 on success, non-zero value on failure (e.g., file cannot be opened or written).
 */
uint8_t write_state(const char* filename, const state_t* state);

/**
 * @brief Reads and deserializes the torrent state from a file.
 *
 * This function loads a previously saved torrent state from the specified file,
 * which includes information about which pieces have been downloaded. The state
 * is stored in a binary format containing a magic number, version, piece count,
 * piece size, and a bitfield representing downloaded pieces.
 *
 * @param filename The path to the file containing the serialized download state.
 * @return A pointer to a dynamically allocated state_t structure containing the
 *         deserialized state, or NULL if the file cannot be read or the state is invalid.
 *         The caller is responsible for freeing the allocated memory.
 */
state_t* read_state(const char* filename);

/**
 * Initializes the state of a torrent download by reading from an existing state file or creating a new one
 * if the file does not exist. If an existing state file is found, its content is loaded and returned.
 * If the file does not exist, a new state is created and initialized with given parameters.
 *
 * @param filename The path to the state file. If the file exists, it will be read to initialize the state.
 * @param piece_count The total number of pieces in the torrent. Used when creating a new state.
 * @param piece_size The size of each piece in bytes. Used when creating a new state.
 * @param bitfield A pre-allocated bitfield array representing which pieces have been downloaded. Used when creating a new state.
 * @return A pointer to the initialized state object. If reading the file or allocating memory fails, the behavior depends
 *         on the underlying implementation of the read_state function and memory allocation.
 */
state_t* init_state(const char* filename, uint32_t piece_count, uint32_t piece_size, unsigned char* bitfield);
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
