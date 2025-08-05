#ifndef WHOLE_BENCODE_H
#define WHOLE_BENCODE_H
#include "file.h"
#include "util.h"

/**
 * Decodes a bencoded list and returns it as a linked list structure.
 *
 * @param bencoded_list The bencoded string representing a list to be decoded.
 * @param length A pointer to an unsigned integer where the end index of the
 *               parsed list will be stored. Can be null if not needed.
 * @return A pointer to the head of the linked list containing the decoded elements,
 *         or null if the input cannot be parsed as a valid bencoded list.
 */
ll* decode_bencode_list(const char* bencoded_list, unsigned int* length);

/**
 * Frees all memory associated with a linked list of bencode elements.
 *
 * This function iterates over the linked list, freeing the memory allocated
 * for the `val` field and the list nodes themselves.
 *
 * @param list Pointer to the head of the linked list to be freed. If the list is empty
 *             (nullptr), no operation is performed.
 */
void free_bencode_list(ll* list);

/**
 * Decodes a bencoded string and returns the decoded result as a dynamically allocated string.
 *
 * @param bencoded_value The input bencoded string to be decoded. It must be a properly formatted
 *                       bencoded string that starts with a length descriptor followed by a colon
 *                       and the actual string content.
 * @return A pointer to the decoded string enclosed in double quotes. The caller is responsible
 *         for freeing the allocated memory. Exits the program if the input format is invalid.
 */
char* decode_bencode_string(const char* bencoded_value);

/**
 * Decodes an integer from a bencoded string.
 *
 * @param bencoded_value A pointer to the bencoded string containing the integer.
 * @param endptr A pointer to a pointer that will be set to the character following the decoded integer in the bencoded string, or NULL if not required.
 * @return The decoded integer as an unsigned long. Returns 0 if the value is not a valid integer.
 */
unsigned long decode_bencode_int(const char *bencoded_value, char **endptr);

/**
 * Decodes a bencoded "announce-list" into a linked list structure.
 *
 * @param announce_list A pointer to the bencoded "announce-list" string to be decoded.
 * @param index A pointer to an unsigned long used to track the parsed position. Can be null if not required.
 * @return A pointer to the head node of a linked list representing the decoded "announce-list", or nullptr if decoding fails.
 */
announce_list_ll* decode_announce_list(const char* announce_list, unsigned long* index);

/**
 * Parses a bencoded string and constructs a linked list of file information structures.
 *
 * @param bencode The bencoded string containing file information to parse.
 * @param multiple A flag indicating whether the file entry represents multiple files (true) or a single file (false).
 * @param index A pointer to an unsigned long to store the offset position after parsing
 *              (can be used to track the overall parsing progress). Can be null.
 * @return A pointer to the head of a linked list of files containing their parsed information,
 *         or nullptr if parsing fails.
 */
files_ll* read_info_files(const char* bencode, bool multiple, unsigned long* index);
#endif //WHOLE_BENCODE_H
