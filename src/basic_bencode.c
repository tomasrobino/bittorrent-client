// Project header files
#include "whole_bencode.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Cannot handle lists of lists
ll* decode_bencode_list(const char* bencoded_list, unsigned int* length, const LOG_CODE log_code) {
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
            char *endptr = (char*) bencoded_list+start;
            const int element_length = (int)decode_bencode_int(bencoded_list+start, &endptr, log_code);
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
        if (length != nullptr) *length += start;
        return head;
    }
    return nullptr;
}

void free_bencode_list(ll* list) {
    while (list != nullptr) {
        free(list->val);
        ll* next = list->next;
        free(list);
        list = next;
    }
}

char* decode_bencode_string(const char* bencoded_value, const LOG_CODE log_code) {
    // Logging
    FILE* logerr;
    if (log_code >= LOG_ERR) {
        logerr = stderr;
    } else logerr = fopen("/dev/null", "w");
    // Byte strings
    if (is_digit(bencoded_value[0])) {
        char *endptr;
        const int length = (int)strtol(bencoded_value, &endptr, 10);
        if (endptr == bencoded_value) {
            fprintf(logerr, "No valid number found\n");
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
        fprintf(logerr, "Invalid encoded value: %s\n", bencoded_value);
        exit(1);
    }
    fprintf(logerr, "Unsupported formatting\n");
    exit(1);
}

unsigned long decode_bencode_int(const char *bencoded_value, char **endptr, const LOG_CODE log_code) {
    // Logging
    FILE* logerr;
    if (log_code >= LOG_ERR) {
        logerr = stderr;
    } else logerr = fopen("/dev/null", "w");
    if (is_digit(bencoded_value[0])) {
        const unsigned long num = strtol(bencoded_value, endptr, 10);
        if (endptr != nullptr && *endptr == bencoded_value) {
            fprintf(logerr, "Invalid number\n");
            return 0;
        }
        return num;
    }
    return 0;
}