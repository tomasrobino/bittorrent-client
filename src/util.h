#ifndef STRUCTS_H
#define STRUCTS_H
#include <stdint.h>

/**
 * Enumeration to represent different levels of logging.
 *
 * @param LOG_NO No logging.
 * @param LOG_ERR Logs error messages.
 * @param LOG_SUMM Logs a summary of operations or events.
 * @param LOG_FULL Logs detailed information including all operations or events.
 */
typedef enum {
    LOG_NO = 0,
    LOG_ERR = 1,
    LOG_SUMM = 2,
    LOG_FULL = 3
} LOG_CODE;
// This client's client id
#define CLIENT_ID "-IT0001-"

/**
 * A linked list node structure for storing string values
 * 
 * @param next Pointer to the next node in the linked list
 * @param val String value stored in this node 
 */
typedef struct ll {
    struct ll *next;
    char *val;
} ll;

typedef struct ll_uint64_t {
    struct ll_uint64_t *next;
    uint64_t val;
} ll_uint64_t;

/**
 * Structure containing arguments needed for performing a BitTorrent handshake
 * 
 * @param server_addr Network address information for the remote peer
 * @param sockfd Socket file descriptor for the network connection
 * @param info_hash 20-byte SHA1 hash of the info dictionary from the torrent metainfo
 * @param peer_id 20-byte string used as unique identifier for the peer 
 */
typedef struct {
    struct sockaddr *server_addr;
    int32_t sockfd;
    const char *info_hash;
    const char *peer_id;
} handshake_args_t;

/**
 * Checks if the given character is a numeric digit (0-9).
 *
 * @param c The character to be checked.
 * @return true if the character is a digit (0-9), false otherwise.
 */
bool is_digit(char c);
#endif //STRUCTS_H
