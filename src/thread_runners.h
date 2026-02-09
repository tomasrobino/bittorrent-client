#ifndef BITTORRENT_CLIENT_THREAD_RUNNERS_H
#define BITTORRENT_CLIENT_THREAD_RUNNERS_H
#include "file.h"

typedef struct {
    metainfo_t* metainfo;
    const unsigned char* peer_id;
    LOG_CODE log_code;
} torrent_args_t;

void *disk_runner(void *arg);

void *torrent_runner(void *arg);

#endif //BITTORRENT_CLIENT_THREAD_RUNNERS_H