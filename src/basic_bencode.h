//
// Created by Tomas on 5/2/2026.
//

#ifndef BITTORRENT_CLIENT_BASIC_BENCODE_H
#define BITTORRENT_CLIENT_BASIC_BENCODE_H

#include "stdint.h"
#include "util.h"

/**
 * Decodes a bencoded list and returns it as a linked list structure.
 *
 * @param bencoded_list The bencoded string representing a list to be decoded.
 * @param length A pointer to an unsigned integer where the end index of the
 *               parsed list will be stored. Can be null if not needed.
 * @param log_code Controls the verbosity of logging output. Can be LOG_NO (no logging),
 *                 LOG_ERR (error logging), LOG_SUMM (summary logging), or
 *                 LOG_FULL (detailed logging).
 * @return A pointer to the head of the linked list containing the decoded elements,
 *         or null if the input cannot be parsed as a valid bencoded list.
 */
ll* decode_bencode_list(const char* bencoded_list, uint32_t *length, LOG_CODE log_code);

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
 * @param log_code Controls the verbosity of logging output. Can be LOG_NO (no logging),
 *                 LOG_ERR (error logging), LOG_SUMM (summary logging), or
 *                 LOG_FULL (detailed logging).
 * @return A pointer to the decoded string enclosed in double quotes. The caller is responsible
 *         for freeing the allocated memory. Exits the program if the input format is invalid.
 */
char* decode_bencode_string(const char* bencoded_value, LOG_CODE log_code);

/**
 * Decodes an integer from a bencoded string.
 *
 * @param bencoded_value A pointer to the bencoded string containing the integer.
 * @param endptr A pointer to a pointer that will be set to the character following the decoded integer in the bencoded string, or NULL if not required.
 * @param log_code Controls the verbosity of logging output. Can be LOG_NO (no logging),
 *                 LOG_ERR (error logging), LOG_SUMM (summary logging), or
 *                 LOG_FULL (detailed logging).
 * @return The decoded integer as an unsigned long. Returns 0 if the value is not a valid integer.
 */
uint64_t decode_bencode_int(const char *bencoded_value, char **endptr, LOG_CODE log_code);

#endif //BITTORRENT_CLIENT_BASIC_BENCODE_H