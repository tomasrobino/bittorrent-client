//
// Created by Tomas on 10/6/2025.
//

#ifndef PARSING_H
#define PARSING_H
#include "structs.h"

//Check if char is a digit
bool is_digit(char c);

// Decode bencoded list, returns linked list with elements
ll* decode_bencode_list(const char* bencoded_list, unsigned int* length);

// For decoding announce-list
announce_list_ll* decode_announce_list(const char* announce_list, unsigned long* index);

//Decode bencoded string, returns decoded string
char* decode_bencode_string(const char* bencoded_value);

int decode_bencode_int(const char* bencoded_value, char** endptr);

files_ll* read_info_files(const char* bencode, bool multiple, unsigned long* index);

metainfo_t* parse_metainfo(const char* bencoded_value, unsigned long length);

//Returns magnet_data struct of parsed magnet link
magnet_data* process_magnet(const char* magnet);

void free_magnet_data(magnet_data* magnet_data);
#endif //PARSING_H
