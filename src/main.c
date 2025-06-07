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

typedef struct tr_ll {
    struct tr_ll* next;
    char* tracker;
} tr_ll;

typedef struct {
    char* xt; //URN containing file hash
    char* dn; //A filename to display to the user
    long xl;  //The file size, in bytes
    tr_ll* tr; //Tracker URL linked list
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
    char* announce_list;
    char* comment;
    char* created_by;
    unsigned int creation_date;
    char* encoding;
    info_t* info;
} metainfo_t;

//Check if char is a digit
bool is_digit(char c);

//Decode bencoded string, returns decoded string
char* decode_bencode(const char* bencoded_value);

metainfo_t* parse_metainfo(const char* bencoded_value);

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
                curl_free(data->tr->tracker);
                tr_ll* next = data->tr->next;
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
    } else if (strcmp(command, "decode") == 0) {
        fprintf(stderr, "Logging will appear here.\n");

        const char* encoded_str = argv[2];
        char* decoded_str = decode_bencode(encoded_str);
        printf("%s\n", decoded_str);
        free(decoded_str);
    } else {
        fprintf(stderr, "Unknown command: %s\n", command);
        return 1;
    }
    return 0;
}

bool is_digit(const char c) {
    return c >= '0' && c <= '9';
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
            // Memory leak false positive
            // ReSharper disable once CppDFAMemoryLeak
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
            // Memory leak false positive
            // ReSharper disable once CppDFAMemoryLeak
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

metainfo_t* parse_metainfo(const char* bencoded_value) {
    unsigned long length = strlen(bencoded_value);
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
                fprintf(stderr, "No valid number found in announce section\n");
                return nullptr;
            }
            //start+=3;
            start = strchr(bencoded_value+start, ':') - bencoded_value + 1;
            metainfo->announce = malloc(sizeof(char)*amount+1);
            strncpy(metainfo->announce, bencoded_value+start, amount);
            metainfo->announce[amount] = '\0';
            start+=amount;
        } else return nullptr;

        //Reading announce-list
        if ( (metainfo->announce_list = strstr(bencoded_value+start, "announce-list")) != nullptr) {
            start = metainfo->announce_list-bencoded_value + 13;
            char *endptr = nullptr;
            const int amount = (int)strtol(bencoded_value+start, &endptr, 10);
            if (endptr == bencoded_value+start) {
                fprintf(stderr, "No valid number found in announce-list section\n");
                return nullptr;
            }
            start = strchr(bencoded_value+start, ':') - bencoded_value + 1;
            /*
            metainfo->announce_list = malloc(sizeof(char)*amount+1);
            strncpy(metainfo->announce_list, bencoded_value+start, amount);
            metainfo->announce_list[amount] = '\0';
            */
            start+=amount;
        }

        // Reading comment
        if ( (metainfo->comment = strstr(bencoded_value+start, "comment")) != nullptr) {
            start = metainfo->comment-bencoded_value + 8;
            char *endptr = nullptr;
            const int amount = (int)strtol(bencoded_value+start, &endptr, 10);
            if (endptr == bencoded_value+start) {
                fprintf(stderr, "No valid number found in comment section\n");
                return nullptr;
            }
            //start+=3;
            start = strchr(bencoded_value+start, ':') - bencoded_value + 1;
            metainfo->comment = malloc(sizeof(char)*amount+1);
            strncpy(metainfo->comment, bencoded_value+start, amount);
            metainfo->comment[amount] = '\0';
            start+=amount;
        }

        // Reading created by
        if ( (metainfo->created_by = strstr(bencoded_value+start, "comment")) != nullptr) {
            start = metainfo->created_by-bencoded_value + 8;
            char *endptr = nullptr;
            const int amount = (int)strtol(bencoded_value+start, &endptr, 10);
            if (endptr == bencoded_value+start) {
                fprintf(stderr, "No valid number found in created by section\n");
                return nullptr;
            }
            //start+=3;
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
                fprintf(stderr, "No valid number found in encoding section\n");
                return nullptr;
            }
            //start+=3;
            start = strchr(bencoded_value+start, ':') - bencoded_value + 1;
            metainfo->encoding = malloc(sizeof(char)*amount+1);
            strncpy(metainfo->encoding, bencoded_value+start, amount);
            metainfo->encoding[amount] = '\0';
            start+=amount;
        }

        char* info_index = strstr(bencoded_value+start, "info");
        if (info_index != nullptr) {

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
    tr_ll* head = malloc(sizeof(tr_ll));
    head->next = nullptr;
    head->tracker = nullptr;
    tr_ll* current = head;
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
                            fprintf(stderr, "No valid number found for xl attribute\n");
                            exit(1);
                        }
                        fprintf(stdout, "xl:\n%ld\n", data->xl);
                        break;
                    case tr:
                        if (tracker_count > 0) {
                            current->next = malloc(sizeof(tr_ll));
                            current = current->next;
                            current->next = nullptr;
                        }
                        current->tracker = malloc(sizeof(char)*(i-start+1));
                        current->tracker = curl_easy_unescape(nullptr, magnet+start, i-start, nullptr);
                        tracker_count++;
                        fprintf(stdout, "tr:\n%s\n", current->tracker);
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