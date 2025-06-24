#ifndef CONNECTION
#define CONNECTION
#include <netinet/in.h>

#include "structs.h"

// Splits protocol, host and port into struct
address_t* split_address(const char* address);
/** Converts domain to IP. Supports UDP, HTTP, and HTTPS; IPv4 and IPv6
 * @param address The domain as a string
 * @returns The IP of the domain as a string
 **/
char* url_to_ip(address_t address);
connect_response_t* connect_request_udp(struct sockaddr_in* server_addr, int sockfd);
void download(const char* raw_address);

#endif //CONNECTION
