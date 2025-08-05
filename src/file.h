#ifndef FILE_H
#define FILE_H

#include "structs.h"

typedef struct files_ll {
    struct files_ll* next;
    unsigned long length;
    ll* path;
} files_ll;

typedef struct announce_list_ll {
    struct announce_list_ll* next;
    ll* list;
} announce_list_ll;

typedef struct {
    files_ll* files;
    unsigned long length;
    char* name;
    unsigned int piece_length;
    unsigned int piece_number;
    char* pieces;
    bool priv;
    char hash[21];
    char human_hash[41];
} info_t;

typedef struct {
    char* announce;
    announce_list_ll* announce_list;
    char* comment;
    char* created_by;
    unsigned int creation_date;
    char* encoding;
    info_t* info;
} metainfo_t;

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
