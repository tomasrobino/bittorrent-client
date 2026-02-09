#ifndef BITTORRENT_CLIENT_DISK_H
#define BITTORRENT_CLIENT_DISK_H
#include <stdint.h>

#include "downloading_types.h"

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

#endif //BITTORRENT_CLIENT_DISK_H