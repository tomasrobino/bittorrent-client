#ifndef BITTORRENT_CLIENT_THREAD_RUNNERS_H
#define BITTORRENT_CLIENT_THREAD_RUNNERS_H
#include "file.h"

/**
 * @brief Arguments structure for torrent worker threads.
 * 
 * This structure encapsulates all necessary parameters for running a torrent
 * download thread. It is passed to torrent_runner() when creating new threads
 * to handle individual torrent operations.
 * 
 * @note The metainfo and peer_id pointers are not owned by this structure.
 *       The caller must ensure they remain valid for the lifetime of the thread.
 */
typedef struct {
    metainfo_t *metainfo; /**< Pointer to torrent metadata including trackers and file information */
    const unsigned char *peer_id; /**< 20-byte unique identifier for this BitTorrent client instance */
    uint8_t thread_id; /**< Numeric identifier for this torrent worker thread (0-based index) */
    LOG_CODE log_code; /**< Logging verbosity level for this thread's operations */
} torrent_args_t;

void *disk_runner(void *arg);

void *torrent_runner(void *arg);

#endif //BITTORRENT_CLIENT_THREAD_RUNNERS_H