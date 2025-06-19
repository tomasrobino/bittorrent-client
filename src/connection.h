#ifndef CONNECTION
#define CONNECTION
#include "structs.h"
#include <sys/socket.h>

// Splits protocol, host and port into struct
address_t* split_address(const char* address);
char* url_to_ip(address_t address);
connect_response_t* connect_udp(struct sockaddr* server_addr, int sockfd, unsigned int transaction_id);
void socketize(unsigned short port, const char* ip, const char* data);
void download(const char* address);

#endif //CONNECTION
