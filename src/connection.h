#ifndef CONNECTION
#define CONNECTION
#include <netinet/in.h>

#include "structs.h"

// Splits protocol, host and port into struct
address_t* split_address(const char* address);
char* url_to_ip(address_t address);
connect_response_t* connect_udp(struct sockaddr_in* server_addr, int sockfd, unsigned int transaction_id);
void download(const char* raw_address);

#endif //CONNECTION
