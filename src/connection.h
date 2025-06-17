//
// Created by Tomas on 10/6/2025.
//

#ifndef CONNECTION
#define CONNECTION
#include "structs.h"

address_t* parse_address(const char* address);
void send_data_udp(unsigned short port, const char* ip, const char* data);
#endif //CONNECTION
