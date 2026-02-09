// Project header files
#include <errno.h>

#include "parsing.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>

#include "predownload_udp.h"
#include "magnet.h"
#include "thread_runners.h"

int32_t main(const int32_t argc, char* argv[]) {
    // Generating peer id
    unsigned char* peer_id = calloc(21, 1);
    memcpy(peer_id, CLIENT_ID, 9);
    peer_id[20] = '\0';
    arc4random_buf(peer_id+8, 12);

    // Disable output buffering
    setbuf(stdout, nullptr);
    setbuf(stderr, nullptr);
    // Redirecting console output to output.log
    //freopen("output.log", "w", stdout);

    if (argc < 3) {
        fprintf(stderr, "Usage: bittorrent-client.sh <command> <args>\n");
        free(peer_id);
        return 1;
    }
    // Logging
    int32_t log_code = LOG_SUMM;
    if (argc > 3) {
        if (strcmp("error", argv[3]) == 0) {
            log_code = LOG_ERR;
        } else if (strcmp("summary", argv[3]) == 0) {
            log_code = LOG_SUMM;
        } else if (strcmp("full", argv[3]) == 0) {
            log_code = LOG_FULL;
        } else log_code = LOG_NO;
    }

    const char* command = argv[1];
    if (log_code >= LOG_ERR) fprintf(stderr, "Logging will appear here.\n");

    if (strcmp(command, "magnet") == 0) {
        const char* magnet_link = argv[2];
        //Check if the link begins correctly
        if (strncmp(magnet_link, "magnet:", 7) == 0) {
            magnet_data* data = process_magnet(magnet_link+7, 0);


            //Freeing magnet data
            free_magnet_data(data);
        } else {
            if (log_code >= LOG_ERR) fprintf(stderr, "Invalid link: %s\n", command);
            free(peer_id);
            return 1;
        }
    } else if (strcmp(command, "file") == 0) {
        char* buffer = nullptr;
        uint64_t length = 0;
        FILE* f = fopen(argv[2], "r");
        if (f) {
            fseek (f, 0, SEEK_END);
            length = ftell (f);
            fseek (f, 0, SEEK_SET);
            buffer = malloc(length);
            if (buffer) {
                fread (buffer, 1, length, f);
            }
            fclose (f);
        } else if (log_code >= LOG_ERR) fprintf(stderr, "Torrent file not found\n");

        if (buffer && length != 0) {
            errno = 0;
            const int32_t mk_res = mkdir("download-folder", 0755);
            if (mk_res == -1 && errno != 0 && errno != 17) {
                if (log_code >= LOG_ERR) fprintf(stderr, "Error when creating torrent directory. Errno: %d", errno);
                free(peer_id);
                return 2;
            }
            metainfo_t* metainfo = parse_metainfo(buffer, length, log_code);
            if (metainfo != nullptr) {
                pthread_t disk_thread;
                pthread_create(&disk_thread, nullptr, disk_runner, nullptr);

                torrent_args_t* torrent_args = calloc(sizeof(torrent_args_t), 1);
                torrent_args->metainfo = metainfo;
                torrent_args->peer_id = peer_id;
                torrent_args->log_code = log_code;
                pthread_t torrent_thread;
                pthread_create(&torrent_thread, nullptr, torrent_runner, torrent_args);


                pthread_join(torrent_thread, nullptr);
                pthread_join(disk_thread, nullptr);
                free(torrent_args);
                free_metainfo(metainfo);
            }
            free(buffer);
        } else if (log_code >= LOG_ERR) fprintf(stderr, "File reading buffer error");
    } else {
        if (log_code >= LOG_ERR) fprintf(stderr, "Unknown command: %s\n", command);
        free(peer_id);
        return 1;
    }
    free(peer_id);
    return 0;
}

