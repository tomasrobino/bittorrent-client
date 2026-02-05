#ifndef PARSING_H
#define PARSING_H
#include "file.h"
#include "util.h"

/**
 * Decodes a bencoded "announce-list" into a linked list structure.
 *
 * @param announce_list A pointer to the bencoded "announce-list" string to be decoded.
 * @param index A pointer to an unsigned long used to track the parsed position. Can be null if not required.
 * @param log_code Controls the verbosity of logging output. Can be LOG_NO (no logging),
 *                 LOG_ERR (error logging), LOG_SUMM (summary logging), or
 *                 LOG_FULL (detailed logging).
 * @return A pointer to the head node of a linked list representing the decoded "announce-list", or nullptr if decoding fails.
 */
announce_list_ll* decode_announce_list(const char* announce_list, uint64_t *index, LOG_CODE log_code);

/**
 * Parses a bencoded string and constructs a linked list of file information structures.
 *
 * @param bencode The bencoded string containing file information to parse.
 * @param multiple A flag indicating whether the file entry represents multiple files (true) or a single file (false).
 * @param index A pointer to an unsigned long to store the offset position after parsing
 *              (can be used to track the overall parsing progress). Can be null.
 * @param log_code Controls the verbosity of logging output. Can be LOG_NO (no logging),
 *                 LOG_ERR (error logging), LOG_SUMM (summary logging), or
 *                 LOG_FULL (detailed logging).
 * @return A pointer to the head of a linked list of files containing their parsed information,
 *         or nullptr if parsing fails.
 */
files_ll* read_info_files(const char* bencode, bool multiple, uint64_t *index, LOG_CODE log_code);
#endif //PARSING_H
