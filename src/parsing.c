// Project header files
#include "parsing.h"

#include <stdlib.h>
#include <string.h>

#include "basic_bencode.h"

announce_list_ll* decode_announce_list(const char* announce_list, uint64_t *index, const LOG_CODE log_code) {
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

        uint32_t element_num = 0;
        uint64_t start = 1;
        while (announce_list[start] != 'e') {
            if (element_num > 0) {
                current->next = malloc(sizeof(announce_list_ll));
                current = current->next;
                current->next = nullptr;
            }
            uint32_t length = 0;
            uint32_t* length_ptr = &length;
            current->list = decode_bencode_list(announce_list+start, length_ptr, log_code);
            start+=length+1;
            element_num++;
        }
        if (index != nullptr) *index += start;
        return head;
    }
    return nullptr;
}

files_ll* read_info_files(const char* bencode, const bool multiple, uint64_t *index, const LOG_CODE log_code) {
    files_ll *head = malloc(sizeof(files_ll));
    head->path = nullptr;
    head->next = nullptr;
    head->file_ptr = nullptr;
    files_ll *current = head;
    uint32_t start = 1;
    uint32_t* start_ptr = &start;
    uint32_t element_num = 0;
    int64_t acc = 0;

    while (bencode[start] != 'e') {
        if (element_num > 0) {
            current->next = malloc(sizeof(files_ll));
            current = current->next;
            current->next = nullptr;
            current->path = nullptr;
            head->file_ptr = nullptr;
        }

        char* parse_index;
        // Parsing length
        if ( (parse_index = strstr(bencode+start, "length")) != nullptr) {
            start = parse_index-bencode + 7;
            char *endptr = nullptr;
            current->length = (int64_t)decode_bencode_int(bencode+start, &endptr, log_code);
            start = strchr(bencode+start, ':') - bencode + 1;
        } else return nullptr;

        // Skipping possible md5sum

        if (multiple) { // Parsing path
            if ( (parse_index = strstr(bencode+start, "path")) != nullptr) {
                start = parse_index-bencode + 4;
                current->path = decode_bencode_list(bencode+start, start_ptr, log_code);
                start+=2;
            } else return nullptr;
        } else { // Parsing name
            if ( (parse_index = strstr(bencode+start, "name")) != nullptr) {
                start = parse_index-bencode + 4;
                char *endptr = nullptr;
                const int32_t amount = (int32_t) decode_bencode_int(bencode+start, &endptr, log_code);
                current->path = malloc(sizeof(ll));
                current->path->next = nullptr;
                current->path->val = malloc(sizeof(char)*(amount+1));
                strncpy(current->path->val, endptr+1, amount);
                current->path->val[amount] = '\0';
                start+=amount;
            } else return nullptr;
        }

        current->byte_index = acc;
        acc += current->length;
        element_num++;

        if (!multiple) break;
    }

    if (index != nullptr) *index += start;
    return head;
}