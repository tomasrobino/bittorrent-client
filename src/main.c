// Project header files
#include "whole_bencode.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(const int argc, char* argv[]) {
    // Disable output buffering
    setbuf(stdout, nullptr);
    setbuf(stderr, nullptr);

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
            metainfo_t* metainfo = parse_metainfo(buffer, length);
            free(buffer);
        } else fprintf(stderr, "File reading buffer error");
    } else {
        fprintf(stderr, "Unknown command: %s\n", command);
        return 1;
    }
    return 0;
}

