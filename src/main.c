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

int main(const int argc, char* argv[]) {
    // Generating peer id
    char peer_id[21] = CLIENT_ID;
    peer_id[20] = '\0';
    arc4random_buf(peer_id+8, 12);

    // Disable output buffering
    setbuf(stdout, nullptr);
    setbuf(stderr, nullptr);
    // Buffer variables for logging options

    if (argc < 3) {
        fprintf(stderr, "Usage: bittorrent-client.sh <command> <args>\n");
        return 1;
    }

    const char* command = argv[1];
    fprintf(stderr, "Logging will appear here.\n");

    if (strcmp(command, "magnet") == 0) {
        const char* magnet_link = argv[2];
        //Check if the link begins correctly
        if (strncmp(magnet_link, "magnet:", 7) == 0) {
            magnet_data* data = process_magnet(magnet_link+7);


            //Freeing magnet data
            free_magnet_data(data);
        } else {
            fprintf(stderr, "Invalid link: %s\n", command);
            return 1;
        }
    } else if (strcmp(command, "file") == 0) {
        char* buffer = nullptr;
        unsigned long length = 0;
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
        } else fprintf(stderr, "Torrent file not found");

        if (buffer && length != 0) {
            errno = 0;
            const int mk_res = mkdir("download-folder", 0755);
            if (mk_res == -1 && errno != 0 && errno != 17) {
                fprintf(stderr, "Error when creating torrent directory. Errno: %d", errno);
                return 2;
            }
            metainfo_t* metainfo = parse_metainfo(buffer, length);
            if (metainfo != nullptr) {
                if (argc > 3) {
                    int log_code = LOG_NO;
                    if (strcmp("no", argv[3]) == 0) {
                        log_code = LOG_NO;
                    } else if (strcmp("error", argv[3]) == 0) {
                        log_code = LOG_ERR;
                    } else if (strcmp("summary", argv[3]) == 0) {
                        log_code = LOG_SUMM;
                    } else if (strcmp("full", argv[3]) == 0) {
                        log_code = LOG_FULL;
                    }
                    torrent(*metainfo, peer_id, log_code);
                } else torrent(*metainfo, peer_id, LOG_NO);

                free_metainfo(metainfo);
            }
            free(buffer);
        } else fprintf(stderr, "File reading buffer error");
    } else {
        fprintf(stderr, "Unknown command: %s\n", command);
        return 1;
    }
    return 0;
}
