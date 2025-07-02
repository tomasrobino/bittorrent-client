#ifndef CONNECTION
#define CONNECTION
#include <netinet/in.h>

#include "structs.h"

// Splits protocol, host and port into struct
address_t* split_address(const char* address);
void shuffle_address_array(address_t* array[], int length);
/** Converts domain to IP. Supports UDP, HTTP, and HTTPS; IPv4 and IPv6
 * @param address The domain as a string
 * @returns The IP of the domain as a string
 **/
char* url_to_ip(address_t* address);
int* try_request_udp(int amount, const int sockfd[], const void *req[], size_t req_size, const struct sockaddr *server_addr[]);
uint64_t connect_request_udp(const struct sockaddr *server_addr[], const int sockfd[], int amount, int* successful_index);
announce_response_t* announce_request_udp(
    const struct sockaddr *server_addr,
    const int sockfd,
    uint64_t connection_id,
    char info_hash[],
    char peer_id[],
    uint64_t downloaded,
    uint64_t left,
    uint64_t uploaded,
    uint32_t event,
    uint32_t key,
    uint16_t port
);
uint64_t connect_udp(int amount, announce_list_ll* current, int* successful_index_pt, connection_data_t* connection_data);
void download(metainfo_t metainfo);

#endif //CONNECTION
