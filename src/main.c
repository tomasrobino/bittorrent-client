// Project header files
#include <errno.h>

#include "whole_bencode.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "predownload_udp.h"
#include "downloading.h"
#include "magnet.h"

int32_t main(const int32_t argc, char* argv[]) {
    // Generating peer id
    unsigned char peer_id[21] = CLIENT_ID;
    peer_id[20] = '\0';
    arc4random_buf(peer_id+8, 12);

    // Disable output buffering
    setbuf(stdout, nullptr);
    setbuf(stderr, nullptr);
    // Redirecting console output to output.log
    //freopen("output.log", "w", stdout);

    if (argc < 3) {
        fprintf(stderr, "Usage: bittorrent-client.sh <command> <args>\n");
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
                return 2;
            }
            metainfo_t* metainfo = parse_metainfo(buffer, length, log_code);
            if (metainfo != nullptr) {
                torrent(*metainfo, peer_id, log_code);
                free_metainfo(metainfo);
            }
            free(buffer);
        } else if (log_code >= LOG_ERR) fprintf(stderr, "File reading buffer error");
    } else {
        if (log_code >= LOG_ERR) fprintf(stderr, "Unknown command: %s\n", command);
        return 1;
    }
    return 0;
}
