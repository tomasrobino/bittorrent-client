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

//Check if char is a digit
bool is_digit(char c);
//Decode magnet link


//Decode bencoded string, returns decoded string
//char* decode_bencode(const char* bencoded_value);
/*
int main(const int argc, char* argv[]) {
	// Disable output buffering
	setbuf(stdout, nullptr);
 	setbuf(stderr, nullptr);

    if (argc < 3) {
        fprintf(stderr, "Usage: bittorrent-client.sh <command> <args>\n");
        return 1;
    }

    const char* command = argv[1];

    if (strcmp(command, "decode") == 0) {
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
*/

//Returns magnet_data struct of parsed magnet link
magnet_data* process_magnet(const char* magnet);

bool is_digit(const char c) {
    return c >= '0' && c <= '9';
}

/*
char* decode_bencode(const char* bencoded_value) {
    // Byte strings
    if (is_digit(bencoded_value[0])) {
        const int length = atoi(bencoded_value);
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
*/

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
            free(data);
        } else {
            fprintf(stderr, "Invalid link: %s\n", command);
            return 1;
        }
    } else {
        fprintf(stderr, "Unknown command: %s\n", command);
        return 1;
    }
    return 0;
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
                        data->xl = atoi(magnet+start);
                        fprintf(stdout, "xl:\n%ld\n", data->xl);
                        break;
                    case tr:
                        if (tracker_count > 0) {
                            current->next = malloc(sizeof(tr_ll));
                            current = current->next;
                        }
                        current->tracker = malloc(sizeof(char)*(i-start+1));
                        current->tracker = curl_easy_unescape(nullptr, magnet+start, i-start, nullptr);
                        tracker_count++;
                        fprintf(stdout, "tr:\n%s\n", current->tracker);
                        break;
                    case ws:
                        break;
                    case as:
                        break;
                    case xs:
                        break;
                    case kt:
                        break;
                    case mt:
                        break;
                    case so:
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
    return data;
}