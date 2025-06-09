#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

typedef enum {
    null = 20,
    xt,
    dn,
    xl,
    tr,
    ws,
    as,
    xs,
    kt,
    mt,
    so
} Magnet_Attributes;

typedef struct ll {
    struct ll *next;
    char *val;
} ll;

typedef struct {
    char* xt; //URN containing file hash
    char* dn; //A filename to display to the user
    long xl;  //The file size, in bytes
    ll* tr; //Tracker URL linked list
    char* ws; //The payload data served over HTTP(S)
    char* as; //Refers to a direct download from a web server
    char* xs; //Either an HTTP (or HTTPS, FTP, FTPS, etc.) download source for the file pointed to by the Magnet link,
              //the address of a P2P source for the file or the address of a hub (in the case of DC++),
              //by which a client tries to connect directly, asking for the file and/or its sources
    char* kt; //Specifies a string of search keywords to search for in P2P networks, rather than a particular file
    char* mt; //Link to the metafile that contains a list of magnets
    char* so; //Lists specific files torrent clients should download,
              //indicated as individual or ranges (inclusive) of file indexes.
} magnet_data;

typedef struct files_ll {
    struct files_ll* next;
    long length;
    char* path;
} files_ll;

typedef struct announce_list_ll {
    struct announce_list_ll* next;
    ll* list;
} announce_list_ll;

typedef struct {
    files_ll* files;
    long length;
    char* name;
    unsigned int piece_length;
    long pieces;
    bool private;
} info_t;

typedef struct {
    char* announce;
    announce_list_ll* announce_list;
    char* comment;
    char* created_by;
    unsigned int creation_date;
    char* encoding;
    info_t* info;
} metainfo_t;

//Check if char is a digit
bool is_digit(char c);

// Decode bencoded list, returns linked list with elements
ll* decode_bencode_list(const char* bencoded_list, unsigned int* length);

// For decoding announce-list
announce_list_ll* decode_announce_list(const char* announce_list, unsigned long* index);

//Decode bencoded string, returns decoded string
char* decode_bencode(const char* bencoded_value);

files_ll* read_files(char* bencode, bool multiple);

metainfo_t* parse_metainfo(const char* bencoded_value, unsigned long length);

//Returns magnet_data struct of parsed magnet link
magnet_data* process_magnet(const char* magnet);

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
            if (data->xt != nullptr) free(data->xt);
            if (data->dn != nullptr) free(data->dn);
            while (data->tr != nullptr) {
                curl_free(data->tr->val);
                ll* next = data->tr->next;
                free(data->tr);
                data->tr = next;
            }
            if (data->ws != nullptr) curl_free(data->ws);
            if (data->as != nullptr) curl_free(data->as);
            if (data->xs != nullptr) curl_free(data->xs);
            if (data->kt != nullptr) free(data->kt);
            if (data->mt != nullptr) curl_free(data->mt);
            if (data->so != nullptr) free(data->so);
            if (data != nullptr) free(data);
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

bool is_digit(const char c) {
    return c >= '0' && c <= '9';
}

// Cannot handle lists of lists
ll* decode_bencode_list(const char* bencoded_list, unsigned int* length) {
    // Checking if the beginning is valid
    if (bencoded_list[0] == 'l') {
        unsigned long start = 1;
        ll* head;
        ll* current;
        unsigned int element_num = 0;

        // If the list isn't empty
        if (bencoded_list[1] != 'e') {
            head = malloc(sizeof(ll));
            head->val = nullptr;
            head->next = nullptr;
            current = head;
        } else return nullptr;

        while (bencoded_list[start] != 'e') {
            char *endptr;
            int element_length = (int)strtol(bencoded_list+start, &endptr, 10);
            if (endptr == bencoded_list) {
                fprintf(stderr, "Invalid list element length\n");
                exit(1);
            }
            // endptr points to ":", start is moved to the character after it, which is where the data begins
            start = endptr-bencoded_list+1;
            // Copying data

            // True always except for the first element
            if (element_num > 0) {
                current->next = malloc(sizeof(ll));
                current = current->next;
                current->next = nullptr;
            }
            current->val = malloc(sizeof(char) * (element_length + 1));
            strncpy(current->val, bencoded_list+start, element_length);
            current->val[element_length] = '\0';
            element_num++;
            start+=element_length;
        }

        // If passed a pointer as argument, stores end index in it
        if (length != nullptr) *length = start;
        return head;
    }
    return nullptr;
}

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

char* decode_bencode(const char* bencoded_value) {
    // Byte strings
    if (is_digit(bencoded_value[0])) {
        char *endptr;
        const int length = (int)strtol(bencoded_value, &endptr, 10);
        if (endptr == bencoded_value) {
            fprintf(stderr, "No valid number found\n");
            exit(1);
        }
        const char* colon_index = strchr(bencoded_value, ':');
        if (colon_index != NULL) {
            const char* start = colon_index + 1;
            char* decoded_str = malloc(sizeof(char)*length + 3);
            decoded_str[0] = '"';
            strncpy(decoded_str+1, start, length);
            decoded_str[length+1] = '"';
            decoded_str[length+2] = '\0';
            return decoded_str;
        }
        fprintf(stderr, "Invalid encoded value: %s\n", bencoded_value);
        exit(1);
    }

    // Integers
    if (bencoded_value[0] == 'i') {
        const char* end_index = strchr(bencoded_value, 'e');
        if (end_index != NULL) {
            const int length = (int) (end_index - bencoded_value);
            char* decoded_str = malloc(sizeof(char)*length);
            strncpy(decoded_str, bencoded_value+1, length-1);
            decoded_str[length] = '\0';
            return decoded_str;
        }
        fprintf(stderr, "Invalid encoded value: %s\n", bencoded_value);
        exit(1);
    }

    //Lists
    if (bencoded_value[0] == 'l') {

    }

    //Dictionaries
    if (bencoded_value[0] == 'd') {

    }

    fprintf(stderr, "Unsupported formatting\n");
    exit(1);
}

files_ll* read_files(char* bencode, bool multiple) {
    files_ll* files;
    unsigned int start = 1;
}

metainfo_t* parse_metainfo(const char* bencoded_value, const unsigned long length) {
    // It MUST begin with 'd' and end with 'e'
    if (bencoded_value[0] == 'd' && bencoded_value[length-1] == 'e') {
        metainfo_t* metainfo = malloc(sizeof(metainfo_t));
        unsigned long start = 0;
        // Reading announce
        if ( (metainfo->announce = strstr(bencoded_value+start, "announce")) != nullptr) {
            start = metainfo->announce-bencoded_value + 8;
            char *endptr = nullptr;
            const int amount = (int)strtol(bencoded_value+start, &endptr, 10);
            if (endptr == bencoded_value+start) {
                fprintf(stderr, "Invalid length found in announce section\n");
                return nullptr;
            }
            start = strchr(bencoded_value+start, ':') - bencoded_value + 1;
            metainfo->announce = malloc(sizeof(char)*amount+1);
            strncpy(metainfo->announce, bencoded_value+start, amount);
            metainfo->announce[amount] = '\0';
            start+=amount;
        } else return nullptr;

        //Reading announce-list
        char* announce_list_index;
        if ( (announce_list_index = strstr(bencoded_value+start, "announce-list")) != nullptr) {
            start = announce_list_index-bencoded_value + 13;
            unsigned long* start_ptr = &start;
            metainfo->announce_list = decode_announce_list(bencoded_value+start, start_ptr);
        }

        // Reading comment
        if ( (metainfo->comment = strstr(bencoded_value+start, "comment")) != nullptr) {
            start = metainfo->comment-bencoded_value + 7;
            char *endptr = nullptr;
            const int amount = (int)strtol(bencoded_value+start, &endptr, 10);
            if (endptr == bencoded_value+start) {
                fprintf(stderr, "Invalid length found in comment section\n");
                return nullptr;
            }
            start = strchr(bencoded_value+start, ':') - bencoded_value + 1;
            metainfo->comment = malloc(sizeof(char)*amount+1);
            strncpy(metainfo->comment, bencoded_value+start, amount);
            metainfo->comment[amount] = '\0';
            start+=amount;
        }

        // Reading created by
        if ( (metainfo->created_by = strstr(bencoded_value+start, "created by")) != nullptr) {
            start = metainfo->created_by-bencoded_value + 10;
            char *endptr = nullptr;
            const int amount = (int)strtol(bencoded_value+start, &endptr, 10);
            if (endptr == bencoded_value+start) {
                fprintf(stderr, "Invalid length found in created by section\n");
                return nullptr;
            }
            start = strchr(bencoded_value+start, ':') - bencoded_value + 1;
            metainfo->created_by = malloc(sizeof(char)*amount+1);
            strncpy(metainfo->created_by, bencoded_value+start, amount);
            metainfo->created_by[amount] = '\0';
            start+=amount;
        }

        // Reading of creation date
        char* creation_date_index = strstr(bencoded_value+start, "creation date");
        if ( creation_date_index != nullptr) {
            start = creation_date_index-bencoded_value + 13 + 1;
            char *endptr = nullptr;
            metainfo->creation_date = (int)strtol(bencoded_value+start, &endptr, 10);
            if (endptr == bencoded_value+start) {
                fprintf(stderr, "No valid number found in creation date section\n");
                return nullptr;
            }
        }

        // Reading encoding
        if ( (metainfo->encoding = strstr(bencoded_value+start, "encoding")) != nullptr) {
            start = metainfo->encoding-bencoded_value + 8;
            char *endptr = nullptr;
            const int amount = (int)strtol(bencoded_value+start, &endptr, 10);
            if (endptr == bencoded_value+start) {
                fprintf(stderr, "Invalid length found in encoding section\n");
                return nullptr;
            }
            start = strchr(bencoded_value+start, ':') - bencoded_value + 1;
            metainfo->encoding = malloc(sizeof(char)*amount+1);
            strncpy(metainfo->encoding, bencoded_value+start, amount);
            metainfo->encoding[amount] = '\0';
            start+=amount;
        }

        // Reading info
        char* info_index = strstr(bencoded_value+start, "info");
        // If info not found, invalid file
        if (info_index != nullptr) {
            // Allocating space for info
            metainfo->info = malloc(sizeof(info_t));
            // Position of "d"
            start = info_index-bencoded_value+4;
            if (bencoded_value[start+1] == '5') { // If multiple files
                info_index = strstr(bencoded_value+start+3, "files");
                // Parsing files

            } else { // If single file
                // Reading name
                if ( (metainfo->info->name = strstr(bencoded_value+start, "name")) != nullptr) {
                    start = metainfo->info->name-bencoded_value + 7;
                    char *endptr = nullptr;
                    const int amount = (int)strtol(bencoded_value+start, &endptr, 10);
                    if (endptr == bencoded_value+start) {
                        fprintf(stderr, "Invalid length found in name section\n");
                        return nullptr;
                    }
                    start = strchr(bencoded_value+start, ':') - bencoded_value + 1;
                    metainfo->info->name = malloc(sizeof(char)*amount+1);
                    strncpy(metainfo->info->name, bencoded_value+start, amount);
                    metainfo->info->name[amount] = '\0';
                    start+=amount;
                }
            }
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
    data->xt=nullptr;
    fprintf(stdout, "magnet data:\n");

    //Initializing tracker linked list
    ll* head = malloc(sizeof(ll));
    head->next = nullptr;
    head->val = nullptr;
    ll* current = head;
    int tracker_count = 0;

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
                        char *endptr = nullptr;
                        data->xl = strtol(magnet+start, &endptr, 10);
                        if (endptr == magnet+start) {
                            fprintf(stderr, "Invalid length found for xl attribute\n");
                            exit(1);
                        }
                        fprintf(stdout, "xl:\n%ld\n", data->xl);
                        break;
                    case tr:
                        if (tracker_count > 0) {
                            current->next = malloc(sizeof(ll));
                            current = current->next;
                            current->next = nullptr;
                        }
                        current->val = malloc(sizeof(char)*(i-start+1));
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