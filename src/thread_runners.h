#ifndef BITTORRENT_CLIENT_THREAD_RUNNERS_H
#define BITTORRENT_CLIENT_THREAD_RUNNERS_H

#include <pthread.h>

#include "file.h"

typedef struct ll_queue_member_t {
    struct ll_queue_member_t* next; /**< Next member of the linked list */
    const files_ll* files_ll; /**< Linked list with all the files in the torrent, sorted by index */
    int64_t byte_index; /**< Starting index in the whole of the torrent */
    int64_t torrent_length; /**< Amount of bytes in the whole torrent */
    uint8_t torrent_id; /**< ID of the torrent this refers to */
} ll_queue_member_t;

typedef struct {
    ll_queue_member_t* head;
    ll_queue_member_t* tail;
    pthread_mutex_t lock;
    pthread_cond_t condition;
} queue_t;

/**
 * @brief Arguments structure for torrent worker threads.
 * 
 * This structure encapsulates all necessary parameters for running a torrent
 * download thread. It is passed to torrent_runner() when creating new threads
 * to handle individual torrent operations.
 * 
 * @note This structure does not own the metainfo and peer_id pointers.
 *       The caller must ensure they remain valid for the lifetime of the thread.
 */
typedef struct {
    metainfo_t *metainfo; /**< Pointer to torrent metadata including trackers and file information */
    const unsigned char *peer_id; /**< 20-byte unique identifier for this BitTorrent client instance */
    uint8_t thread_id; /**< Numeric identifier for this torrent worker thread (0-based index) */
    queue_t* queue;
    LOG_CODE log_code; /**< Logging verbosity level for this thread's operations */
} torrent_args_t;

typedef struct {
    queue_t* queue;
} disk_args_t;

void *disk_runner(void *arg);

void *torrent_runner(void *arg);

#endif //BITTORRENT_CLIENT_THREAD_RUNNERS_H