#ifndef FILE_H
#define FILE_H

#include "util.h"

/**
 * Represents a linked list node for managing file information in a linked list structure.
 * Each node contains data about a file and a pointer to the next node in the list.
 */
typedef struct files_ll {
    struct files_ll* next;
    unsigned long length;
    ll* path;
} files_ll;

/**
 * Represents a nested linked list for managing tracker URLs from announce-list.
 * Each node contains data related to a list of tracker URLs and a pointer to the next node in the list.
 */
typedef struct announce_list_ll {
    struct announce_list_ll* next;
    ll* list;
} announce_list_ll;

/**
 * @brief Represents metadata and content information for a torrent file.
 * Contains details about file(s), piece information, and hash identifiers.
 * 
 * @struct info_t
 */
typedef struct {
    files_ll *files; /**< Linked list of file information for multi-file torrents */
    unsigned long length; /**< Total size of all files in the torrent in bytes */
    char *name; /**< Name of the torrent (single file) or directory name (multiple files) */
    unsigned int piece_length; /**< Size of each piece in bytes */
    unsigned int piece_number; /**< Total number of pieces */
    char *pieces; /**< String containing SHA1 hashes of all pieces concatenated */
    bool priv; /**< Whether the torrent is private (true) or public (false) */
    char hash[21]; /**< 20-byte SHA1 hash of the info dictionary */
    char human_hash[41]; /**< 40-character hex string representation of info hash */
} info_t;

/**
 * @brief Represents the complete metadata structure of a torrent file.
 * Contains primary tracker URL, tracker lists, and other descriptive information.
 *
 * @struct metainfo_t
 */
typedef struct {
    char *announce; /**< Primary tracker URL that clients use to connect */
    announce_list_ll *announce_list; /**< List of backup tracker URLs organized in tiers */
    char *comment; /**< Optional descriptive comment about the torrent */
    char *created_by; /**< Optional string identifying the program used to create the torrent */
    unsigned int creation_date; /**< Unix timestamp when the torrent was created */
    char *encoding; /**< Optional string specifying character encoding of strings in the torrent */
    info_t *info; /**< Pointer to structure containing core torrent content information */
} metainfo_t;

/**
 * Frees the memory associated with the given announce list.
 *
 * @param list The linked list of type announce_list_ll to be freed.
 *             This includes all nodes and nested structures within the list.
 */
void free_announce_list(announce_list_ll* list);

/**
 * Converts a SHA-1 hash represented as a 20-byte array into a hexadecimal string.
 *
 * @param sha1_bytes A pointer to the 20-byte array containing the SHA-1 hash.
 * @param hex_output A pointer to the output buffer where the resulting
 *                   40-character hexadecimal string will be stored.
 *                   The buffer must be at least 41 bytes long to include
 *                   the null-terminator.
 * @return None. The result is stored in the hex_output buffer.
 */
void sha1_to_hex(const unsigned char *sha1_bytes, char *hex_output);

/**
 * Parses the bencoded torrent metainfo data and extracts relevant metadata fields.
 *
 * @param bencoded_value A pointer to the bencoded string containing the torrent metainfo.
 *                       The string must adhere to the bencode format and represent
 *                       the metainfo structure of a torrent file.
 * @param length The length of the bencoded string in bytes.
 *               Must be accurate to avoid memory violations.
 * @return A pointer to a dynamically allocated `metainfo_t` structure containing
 *         extracted metadata, or nullptr if parsing fails or the input is invalid.
 */
metainfo_t *parse_metainfo(const char *bencoded_value, unsigned long length);

/**
 * Frees all dynamically allocated memory associated with a linked list of files.
 *
 * @param list A pointer to the head of the `files_ll` linked list to be freed.
 *             Each element in the list, including its `path` (a linked list of type `ll`),
 *             will be deallocated. The pointer itself and all its subsequent nodes
 *             in the list will be freed.
 * @return None. The memory is freed and the pointer becomes invalid.
 */
void free_info_files_list(files_ll *list);

/**
 * Frees the memory allocated for a `metainfo_t` structure, including all its
 * dynamically allocated members and substructures.
 *
 * @param metainfo A pointer to the `metainfo_t` structure to be freed.
 *                 If `metainfo` or its members are `nullptr`, they will be safely ignored.
 * @return None. The memory is freed and the pointer becomes invalid.
 */
void free_metainfo(metainfo_t *metainfo);
#endif //FILE_H
