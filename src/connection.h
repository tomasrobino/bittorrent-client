#ifndef CONNECTION
#define CONNECTION
#include <netinet/in.h>

#include "structs.h"

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
 * @returns A pointer to the socket file descriptor of the tracker whose connection was successful; or nullptr
 */
int* try_request_udp(int amount, const int sockfd[], const void *req[], size_t req_size, const struct sockaddr *server_addr[]);

/**
 * Sends a connection request to a list of trackers.
 * @param server_addr An array with each tracker's server address
 * @param sockfd An array with socket file descriptors for each tracker
 * @param amount The amount of trackers to try
 * @param successful_index A pointer to allocated memory
 * to store the index in the array of the tracker that successfully connected; or nullptr
 * @returns The connection id successfully returned from the tracker; or 0
 */
uint64_t connect_request_udp(const struct sockaddr *server_addr[], const int sockfd[], int amount, int* successful_index);

/**
 * Attempts to connect to a tracker. Takes either a list of lists or a single one
 * @param amount The amount of trackers
 * @param current A pointer to a linked list with trackers
 * @param successful_index_pt A pointer to allocated memory
 * to store the index in the array of the tracker that successfully connected; or nullptr
 * @param connection_data A pointer to allocated memory to store the details of the connection
 * @return The connection id successfully returned from the tracker; or 0
 */
uint64_t connect_udp(int amount, announce_list_ll* current, int* successful_index_pt, connection_data_t* connection_data);
#endif //CONNECTION
