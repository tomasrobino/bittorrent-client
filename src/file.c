#include <stdlib.h>
#include <string.h>
#include <tgmath.h>

#include "structs.h"
#include "whole_bencode.h"

void free_info_files_list(files_ll* list);

metainfo_t* parse_metainfo(const char* bencoded_value, const unsigned long length) {
    // It MUST begin with 'd' and end with 'e'
    if (bencoded_value[0] == 'd' && bencoded_value[length-1] == 'e') {
        metainfo_t* metainfo = malloc(sizeof(metainfo_t));
        unsigned long start = 0;
        unsigned long* start_ptr = &start;
        // Reading announce
        if ( (metainfo->announce = strstr(bencoded_value+start, "announce")) != nullptr) {
            start = metainfo->announce-bencoded_value + 8;
            const int amount = decode_bencode_int(bencoded_value+start, nullptr);
            start = strchr(bencoded_value+start, ':') - bencoded_value + 1;
            metainfo->announce = malloc(sizeof(char)*(amount+1));
            strncpy(metainfo->announce, bencoded_value+start, amount);
            metainfo->announce[amount] = '\0';
            start+=amount;
        } else return nullptr;

        //Reading announce-list
        char* announce_list_index;
        if ( (announce_list_index = strstr(bencoded_value+start, "announce-list")) != nullptr) {
            start = announce_list_index-bencoded_value + 13;
            metainfo->announce_list = decode_announce_list(bencoded_value+start, start_ptr);
        }

        // Reading comment
        if ( (metainfo->comment = strstr(bencoded_value+start, "comment")) != nullptr) {
            start = metainfo->comment-bencoded_value + 7;
            const int amount = decode_bencode_int(bencoded_value+start, nullptr);
            start = strchr(bencoded_value+start, ':') - bencoded_value + 1;
            metainfo->comment = malloc(sizeof(char)*(amount+1));
            strncpy(metainfo->comment, bencoded_value+start, amount);
            metainfo->comment[amount] = '\0';
            start+=amount;
        }

        // Reading created by
        if ( (metainfo->created_by = strstr(bencoded_value+start, "created by")) != nullptr) {
            start = metainfo->created_by-bencoded_value + 10;
            const int amount = decode_bencode_int(bencoded_value+start, nullptr);
            start = strchr(bencoded_value+start, ':') - bencoded_value + 1;
            metainfo->created_by = malloc(sizeof(char)*(amount+1));
            strncpy(metainfo->created_by, bencoded_value+start, amount);
            metainfo->created_by[amount] = '\0';
            start+=amount;
        }

        // Reading of creation date
        char* creation_date_index = strstr(bencoded_value+start, "creation date");
        if ( creation_date_index != nullptr) {
            start = creation_date_index-bencoded_value + 13 + 1;
            metainfo->creation_date = decode_bencode_int(bencoded_value+start, nullptr);
        }

        // Reading encoding
        if ( (metainfo->encoding = strstr(bencoded_value+start, "encoding")) != nullptr) {
            start = metainfo->encoding-bencoded_value + 8;
            const int amount = decode_bencode_int(bencoded_value+start, nullptr);
            start = strchr(bencoded_value+start, ':') - bencoded_value + 1;
            metainfo->encoding = malloc(sizeof(char)*(amount+1));
            strncpy(metainfo->encoding, bencoded_value+start, amount);
            metainfo->encoding[amount] = '\0';
            start+=amount;
        }

        // Reading info
        char* info_index = strstr(bencoded_value+start, "info");
        // If info not found, invalid file
        if (info_index != nullptr) {
            bool multiple;
            // Allocating space for info
            metainfo->info = malloc(sizeof(info_t));
            // Position of "d"
            start = info_index-bencoded_value+4;

            if (bencoded_value[start+1] == '5') { // If multiple files
                multiple = true;
                metainfo->info->files = read_info_files(bencoded_value+start, multiple, start_ptr);
                // Reading directory name when multiple
                if ( (info_index = strstr(bencoded_value+start, "name")) != nullptr) {
                    start = info_index-bencoded_value + 4;
                    const int amount = decode_bencode_int(bencoded_value+start, nullptr);
                    start = strchr(bencoded_value+start, ':') - bencoded_value + 1;
                    metainfo->info->name = malloc(sizeof(char)*(amount+1));
                    strncpy(metainfo->info->name, bencoded_value+start, amount);
                    metainfo->info->name[amount] = '\0';
                    start+=amount;
                } else return nullptr;

            } else { // If single file
                multiple = false;
                metainfo->info->files = read_info_files(bencoded_value+start, multiple, start_ptr);

                // Skipping md5sum
            }

            // Reading piece length
            info_index = strstr(bencoded_value+start, "piece length");
            if ( info_index != nullptr) {
                start = info_index-bencoded_value + 12 + 1;
                metainfo->info->piece_length = decode_bencode_int(bencoded_value+start, nullptr);
            } else return nullptr;

            // Reading pieces
            if ( (info_index = strstr(bencoded_value+start, "pieces")) != nullptr) {
                start = info_index-bencoded_value + 6;
                const int amount = decode_bencode_int(bencoded_value+start, nullptr);
                start = strchr(bencoded_value+start, ':') - bencoded_value + 1;
                metainfo->info->piece_number = ceil((double) amount / metainfo->info->piece_length);
                metainfo->info->pieces = malloc(sizeof(char)*(amount+1));
                strncpy(metainfo->info->pieces, bencoded_value+start, amount);
                metainfo->info->pieces[amount] = '\0';
                start+=amount;
            } else return nullptr;

            // Skipping private
        } else return nullptr;

        return metainfo;
    }
    return nullptr;
}

void free_info_files_list(files_ll* list) {
    while (list != nullptr) {
        free_bencode_list(list->path);
        files_ll* next = list->next;
        free(list);
        list = next;
    }
}

void free_metainfo(metainfo_t* metainfo) {
    if (metainfo != nullptr) {
        if (metainfo->announce != nullptr) free(metainfo->announce);
        free_announce_list(metainfo->announce_list);
        if (metainfo->comment != nullptr) free(metainfo->comment);
        if (metainfo->created_by != nullptr) free(metainfo->created_by);
        if (metainfo->encoding != nullptr) free(metainfo->encoding);
        if (metainfo->info != nullptr) {
            free_info_files_list(metainfo->info->files);
            if (metainfo->info->name != nullptr) free(metainfo->info->name);
            if (metainfo->info->pieces != nullptr) free(metainfo->info->pieces);
            free(metainfo->info);
        }
        free(metainfo);
    }
}