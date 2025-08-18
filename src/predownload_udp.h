#ifndef CONNECTION
#define CONNECTION
#include <netinet/in.h>

#include "file.h"

/** Amount of times to try connecting to tracker
 * According to the BitTorrent protocol, should be 9, but it's too much
**/
#define MAX_ATTEMPTS 3
#define ANNOUNCE_REQUEST_SIZE 98
#define SCRAPE_REQUEST_SIZE 16
#define MAX_RESPONSE_SIZE 1500

typedef enum {
    HTTP,
    HTTPS,
    UDP
} Protocols;

typedef struct {
    char* host;
    char* port;
    Protocols protocol;
    int ip_version;
} address_t;

typedef struct {
    address_t* split_addr;
    char* ip;
    int sockfd;
    struct sockaddr* server_addr;
} connection_data_t;

typedef struct {
    uint64_t protocol_id; // 0x41727101980 magic constant
    uint32_t action; // 0 connect
    uint32_t transaction_id; // random
} connect_request_t;

typedef struct {
    uint32_t action; // 0 connect
    uint32_t transaction_id; // same as sent
    uint64_t connection_id; // assigned by tracker
} connect_response_t;


/*
Announce request

Offset  Size    Name    Value
0       64-bit integer  connection_id
8       32-bit integer  action          1 // announce
12      32-bit integer  transaction_id
16      20-byte string  info_hash
36      20-byte string  peer_id
56      64-bit integer  downloaded
64      64-bit integer  left
72      64-bit integer  uploaded
80      32-bit integer  event           0 // 0: none; 1: completed; 2: started; 3: stopped
84      32-bit integer  IP address      0 // default. If using IPv6, should always be 0
88      32-bit integer  key
92      32-bit integer  num_want        -1 // default
96      16-bit integer  port
98
*/
typedef struct {
    uint64_t connection_id;
    uint32_t action;
    uint32_t transaction_id;
    char info_hash[20];
    char peer_id[20];
    uint64_t downloaded;
    uint64_t left;
    uint64_t uploaded;
    uint32_t event;
    uint32_t ip;
    uint32_t key;
    uint32_t num_want;
    uint16_t port;
} announce_request_t;

// Represents a peer as returned in an announce response
typedef struct peer_ll {
    char* ip;
    uint16_t port;
    struct peer_ll* next;
} peer_ll;


/*
IPv4 announce response

Offset      Size            Name            Value
0           32-bit integer  action          1 // announce
4           32-bit integer  transaction_id
8           32-bit integer  interval
12          32-bit integer  leechers
16          32-bit integer  seeders
20 + 6 * n  32-bit integer  IP address
24 + 6 * n  16-bit integer  TCP port
20 + 6 * N

IPv6 has instead a stride size of 18, 16 of which belong to "IP address" and 2 to "TCP port"
*/
typedef struct {
    uint32_t action;
    uint32_t transaction_id;
    uint32_t interval;
    uint32_t leechers;
    uint32_t seeders;
    peer_ll* peer_list;
} announce_response_t;

/*
Offset          Size            Name            Value
0               64-bit integer  connection_id
8               32-bit integer  action          2 // scrape
12              32-bit integer  transaction_id
16 + 20 * n     20-byte string  info_hash
16 + 20 * N
*/
typedef struct {
    uint64_t connection_id;
    uint32_t action;
    uint32_t transaction_id;
    const char* info_hash_list;
} scrape_request_t;

/*
Offset      Size            Name            Value
0           32-bit integer  action          2 // scrape
4           32-bit integer  transaction_id
8 + 12 * n  32-bit integer  seeders
12 + 12 * n 32-bit integer  completed
16 + 12 * n 32-bit integer  leechers
8 + 12 * N
*/
typedef struct {
    uint32_t seeders;
    uint32_t completed;
    uint32_t leechers;
} scraped_data_t;

typedef struct {
    uint32_t action;
    uint32_t transaction_id;
    scraped_data_t scraped_data_array[];
} scrape_response_t;

/*
Offset  Size            Name            Value
0       32-bit integer  action          3 // error
4       32-bit integer  transaction_id
8       string  message
*/
typedef struct {
    uint32_t action;
    uint32_t transaction_id;
    char* message;
} error_response;

/** Splits address into protocol, host and port into struct
 * @param address The address to split
 * @returns The split address
 **/
address_t* split_address(const char* address);

/**
 * Shuffles an array of addresses randomly, to comply with the Bittorrent specification
 * @param array The array to shuffle
 * @param length The length of the array
 */
void shuffle_address_array(address_t* array[], int length);
/** Converts domain to IP. Supports UDP, HTTP, and HTTPS; IPv4 and IPv6
 * @param address The domain as a string
 * @returns The IP of the domain as a string
 **/
char* url_to_ip(address_t* address);

/**
 * Attempts to send requests to an array of trackers
 * @param amount The amount of trackers to try
 * @param sockfd An array with socket file descriptors for each tracker
 * @param req An array with connection requests for each tracker
 * @param req_size The size of the connection request
 * @param server_addr An array with each tracker's server address
 * @param log_code Controls the verbosity of logging output. Can be LOG_NO (no logging),
 *                 LOG_ERR (error logging), LOG_SUMM (summary logging), or
 *                 LOG_FULL (detailed logging).
 * @returns A pointer to the socket file descriptor of the tracker whose connection was successful; or nullptr
 */
int* try_request_udp(int amount, const int sockfd[], const void *req[], size_t req_size, const struct sockaddr *server_addr[], LOG_CODE log_code);

/**
 * Sends a connection request to a list of trackers.
 * @param server_addr An array with each tracker's server address
 * @param sockfd An array with socket file descriptors for each tracker
 * @param amount The amount of trackers to try
 * @param successful_index A pointer to allocated memory
 * to store the index in the array of the tracker that successfully connected; or nullptr
 * @param log_code Controls the verbosity of logging output. Can be LOG_NO (no logging),
 *                 LOG_ERR (error logging), LOG_SUMM (summary logging), or
 *                 LOG_FULL (detailed logging).
 * @returns The connection id successfully returned from the tracker; or 0
 */
uint64_t connect_request_udp(const struct sockaddr *server_addr[], const int sockfd[], int amount, int* successful_index, LOG_CODE log_code);

/**
 * Attempts to connect to a tracker. Takes either a list of lists or a single one
 * @param amount The amount of trackers
 * @param current A pointer to a linked list with trackers
 * @param successful_index_pt A pointer to allocated memory
 * to store the index in the array of the tracker that successfully connected; or nullptr
 * @param connection_data A pointer to allocated memory to store the details of the connection
 * @param log_code Controls the verbosity of logging output. Can be LOG_NO (no logging),
 *                 LOG_ERR (error logging), LOG_SUMM (summary logging), or
 *                 LOG_FULL (detailed logging).
 * @return The connection id successfully returned from the tracker; or 0
 */
uint64_t connect_udp(int amount, announce_list_ll* current, int* successful_index_pt, connection_data_t* connection_data, LOG_CODE log_code);

/**
 * Sends an announce request to the tracker
 * @param server_addr Server address
 * @param sockfd Socket file descriptor
 * @param connection_id Returned from connect response of the tracker
 * @param info_hash Info hash of the torrent
 * @param peer_id ID of this peer
 * @param downloaded Amount downloaded
 * @param left Amount left to download
 * @param uploaded Amount uploaded
 * @param event
 * @param key Torrent key
 * @param port Tracker port
 * @param log_code Controls the verbosity of logging output. Can be LOG_NO (no logging),
 *                 LOG_ERR (error logging), LOG_SUMM (summary logging), or
 *                 LOG_FULL (detailed logging).
 * @return The announce response from the server or nullptr if an error ocurred
 */
announce_response_t* announce_request_udp(
    const struct sockaddr *server_addr,
    int sockfd,
    uint64_t connection_id,
    const char info_hash[],
    const char peer_id[],
    uint64_t downloaded,
    uint64_t left,
    uint64_t uploaded,
    uint32_t event,
    uint32_t key,
    uint16_t port,
    LOG_CODE log_code
);

/**
 * Requests torrent scrape data to tracker
 * @param server_addr Server address
 * @param sockfd Socket file descriptor
 * @param connection_id Returned from connect response of the tracker
 * @param info_hash Info hash of all the torrents
 * @param torrent_amount The amount of torrents to ask about
 * @param log_code Controls the verbosity of logging output. Can be LOG_NO (no logging),
 *                 LOG_ERR (error logging), LOG_SUMM (summary logging), or
 *                 LOG_FULL (detailed logging).
 * @return The data returned or nullptr if there's an error
 */
scrape_response_t* scrape_request_udp(const struct sockaddr *server_addr, int sockfd, uint64_t connection_id, const char info_hash[], unsigned int torrent_amount, LOG_CODE log_code);

#endif //CONNECTION
