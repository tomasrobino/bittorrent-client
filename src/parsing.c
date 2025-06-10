// Project header files
#include "whole_bencode.h"

#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tgmath.h>

announce_list_ll* decode_announce_list(const char* announce_list, unsigned long* index) {
    if (announce_list[0] == 'l') {
        announce_list_ll* head;
        announce_list_ll* current;
        // If the list isn't empty
        if (announce_list[1] != 'e') {
            head = malloc(sizeof(announce_list_ll));
            head->list = nullptr;
            head->next = nullptr;
            current = head;
        } else return nullptr;

        unsigned int element_num = 0;
        unsigned long start = 1;
        while (announce_list[start] != 'e') {
            if (element_num > 0) {
                current->next = malloc(sizeof(announce_list_ll));
                current = current->next;
                current->next = nullptr;
            }
            unsigned int length = 0;
            unsigned int* length_ptr = &length;
            current->list = decode_bencode_list(announce_list+start, length_ptr);
            start+=length+1;
            element_num++;
        }
        if (index != nullptr) *index += start;
        return head;
    }
    return nullptr;
}



files_ll* read_info_files(const char* bencode, bool multiple, unsigned long* index) {
    files_ll *head = malloc(sizeof(files_ll));
    head->path = nullptr;
    head->next = nullptr;
    files_ll *current = head;
    unsigned int start = 1;
    unsigned int* start_ptr = &start;
    unsigned int element_num = 0;

    while (bencode[start] != 'e') {
        if (element_num > 0) {
            current->next = malloc(sizeof(files_ll));
            current = current->next;
            current->next = nullptr;
            current->path = nullptr;
        }

        char* parse_index;
        // Parsing length
        if ( (parse_index = strstr(bencode+start, "length")) != nullptr) {
            start = parse_index-bencode + 7;
            char *endptr = nullptr;
            current->length = decode_bencode_int(bencode+start, &endptr);
            start = strchr(bencode+start, ':') - bencode + 1;
        } else return nullptr;

        // Skipping possible md5sum

        if (multiple) { // Parsing path
            if ( (parse_index = strstr(bencode+start, "path")) != nullptr) {
                start = parse_index-bencode + 4;
                current->path = decode_bencode_list(bencode+start, start_ptr);
                start+=2;
            } else return nullptr;
        } else { // Parsing name
            if ( (parse_index = strstr(bencode+start, "name")) != nullptr) {
                start = parse_index-bencode + 4;
                char *endptr = nullptr;
                const int amount = decode_bencode_int(bencode+start, &endptr);
                current->path = malloc(sizeof(ll));
                current->path->next = nullptr;
                current->path->val = malloc(sizeof(char)*(amount+1));
                strncpy(current->path->val, endptr+1, amount);
                current->path->val[amount] = '\0';
                start+=amount;
            } else return nullptr;
        }


        element_num++;

        if (!multiple) break;
    }

    if (index != nullptr) *index += start;
    return head;
}

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

magnet_data* process_magnet(const char* magnet) {
    int length = (int) strlen(magnet);
    int start = 4;
    Magnet_Attributes current_attribute = null;
    magnet_data* data = malloc(sizeof(magnet_data));
    // Initializing pointers to null
    data->xt=nullptr;
    data->dn=nullptr;
    data->tr=nullptr;
    data->ws=nullptr;
    data->as=nullptr;
    data->xs=nullptr;
    data->kt=nullptr;
    data->mt=nullptr;
    data->so=nullptr;

    //Initializing tracker linked list
    ll* head = malloc(sizeof(ll));
    head->next = nullptr;
    head->val = nullptr;
    ll* current = head;
    int tracker_count = 0;
    fprintf(stdout, "magnet data:\n");
    for (int i = 0; i <= length; ++i) {
        if (magnet[i] == '&' || magnet[i] == '?' || magnet[i] == '\0') {
            //Processing previous attribute
            if (current_attribute != null) {
                switch (current_attribute) {
                    case xt:
                        if (strncmp(magnet+start, "urn:btmh:", 9) == 0 || (strncmp(magnet+start, "urn:btih:", 9) == 0 && data->xt == nullptr)) { //SHA-1
                            data->xt = malloc(sizeof(char)*(i-start-9 +1));
                            strncpy(data->xt, magnet+start+9, i-start-9);
                            data->xt[i-start-9] = '\0';
                        } else {
                            fprintf(stderr, "Invalid URN");
                            exit(1);
                        }
                        fprintf(stdout, "xt:\n%s\n", data->xt);
                        break;
                    case dn:
                        data->dn = malloc(sizeof(char)*(i-start+1));
                        for (int j = 0; j < i-start; ++j) {
                            if (magnet[start+j] == '+') {
                                data->dn[j] = ' ';
                            } else data->dn[j] = magnet[start+j];
                        }
                        data->dn[i-start] = '\0';
                        fprintf(stdout, "dn:\n%s\n", data->dn);
                        break;
                    case xl:
                        data->xl = decode_bencode_int(magnet+start, nullptr);
                        fprintf(stdout, "xl:\n%ld\n", data->xl);
                        break;
                    case tr:
                        if (tracker_count > 0) {
                            current->next = malloc(sizeof(ll));
                            current = current->next;
                            current->next = nullptr;
                        }
                        current->val = curl_easy_unescape(nullptr, magnet+start, i-start, nullptr);
                        tracker_count++;
                        fprintf(stdout, "tr:\n%s\n", current->val);
                        break;
                    case ws:
                        data->ws = curl_easy_unescape(nullptr, magnet+start, i-start, nullptr);
                        fprintf(stdout, "ws:\n%s\n", data->ws);
                        break;
                    case as:
                        data->as = curl_easy_unescape(nullptr, magnet+start, i-start, nullptr);
                        fprintf(stdout, "as:\n%s\n", data->as);
                        break;
                    case xs:
                        data->xs = curl_easy_unescape(nullptr, magnet+start, i-start, nullptr);
                        fprintf(stdout, "xs:\n%s\n", data->xs);
                        break;
                    case kt:
                        data->kt = malloc(sizeof(char)*(i-start+1));
                        for (int j = 0; j < i-start; ++j) {
                            if (magnet[start+j] == '+') {
                                data->kt[j] = ' ';
                            } else data->kt[j] = magnet[start+j];
                        }
                        data->kt[i-start] = '\0';
                        fprintf(stdout, "kt:\n%s\n", data->kt);
                        break;
                    case mt:
                        data->mt = curl_easy_unescape(nullptr, magnet+start, i-start, nullptr);
                        fprintf(stdout, "mt:\n%s\n", data->mt);
                        break;
                    case so:
                        data->so = malloc(sizeof(char)*(i-start+1));
                        strncpy(data->so, magnet+start, i-start);
                        data->so[i-start] = '\0';
                        break;
                    default: ;
                }
            }

            //Setting newly found attribute as current

            if (strncmp(magnet+i+1 , "xt", 2) == 0) {
                current_attribute = xt;
            } else if (strncmp(magnet+i+1 , "dn", 2) == 0) {
                current_attribute = dn;
            } else if (strncmp(magnet+i+1 , "xl", 2) == 0) {
                current_attribute = xl;
            } else if (strncmp(magnet+i+1 , "tr", 2) == 0) {
                current_attribute = tr;
            } else if (strncmp(magnet+i+1 , "ws", 2) == 0) {
                current_attribute = ws;
            } else if (strncmp(magnet+i+1 , "as", 2) == 0) {
                current_attribute = as;
            } else if (strncmp(magnet+i+1 , "xs", 2) == 0) {
                current_attribute = xs;
            } else if (strncmp(magnet+i+1 , "kt", 2) == 0) {
                current_attribute = kt;
            } else if (strncmp(magnet+i+1 , "mt", 2) == 0) {
                current_attribute = mt;
            } else if (strncmp(magnet+i+1 , "so", 2) == 0) {
                current_attribute = so;
            }
            start = i+4;
            i+=3;
        }
    }
    data->tr = head;
    return data;
}

void free_magnet_data(magnet_data* magnet_data) {
    if (magnet_data->xt != nullptr) free(magnet_data->xt);
    if (magnet_data->dn != nullptr) free(magnet_data->dn);
    while (magnet_data->tr != nullptr) {
        curl_free(magnet_data->tr->val);
        ll* next = magnet_data->tr->next;
        free(magnet_data->tr);
        magnet_data->tr = next;
    }
    if (magnet_data->ws != nullptr) curl_free(magnet_data->ws);
    if (magnet_data->as != nullptr) curl_free(magnet_data->as);
    if (magnet_data->xs != nullptr) curl_free(magnet_data->xs);
    if (magnet_data->kt != nullptr) free(magnet_data->kt);
    if (magnet_data->mt != nullptr) curl_free(magnet_data->mt);
    if (magnet_data->so != nullptr) free(magnet_data->so);
    if (magnet_data != nullptr) free(magnet_data);
}