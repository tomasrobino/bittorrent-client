#ifndef DOWNLOADING_H
#define DOWNLOADING_H
#include <stdint.h>

#include "structs.h"

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
    uint16_t port
);
void download(metainfo_t metainfo, const char* peer_id);
#endif //DOWNLOADING_H
