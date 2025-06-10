// Project header files
#include "whole_bencode.h"

#include <stdlib.h>
#include <string.h>

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

void free_announce_list(announce_list_ll* list) {
    while (list != nullptr) {
        free_bencode_list(list->list);
        announce_list_ll* next = list->next;
        free(list);
        list = next;
    }
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