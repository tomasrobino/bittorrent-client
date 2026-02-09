#include "thread_runners.h"

#include "downloading.h"

void *disk_runner(void *arg) {

    return nullptr;
}

void *torrent_runner(void *arg) {
    const torrent_args_t* torrent_args = arg;
    torrent(*torrent_args->metainfo, torrent_args->peer_id, torrent_args->log_code);
    return nullptr;
}