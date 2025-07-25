#ifndef WHOLE_BENCODE_H
#define WHOLE_BENCODE_H
#include "structs.h"

//Check if char is a digit
bool is_digit(char c);
// Decode bencoded list, returns linked list with elements
ll* decode_bencode_list(const char* bencoded_list, unsigned int* length);

void free_bencode_list(ll* list);
//Decode bencoded string, returns decoded string
char* decode_bencode_string(const char* bencoded_value);

unsigned long decode_bencode_int(const char *bencoded_value, char **endptr);
// For decoding announce-list
announce_list_ll* decode_announce_list(const char* announce_list, unsigned long* index);

void free_announce_list(announce_list_ll* list);

files_ll* read_info_files(const char* bencode, bool multiple, unsigned long* index);

metainfo_t* parse_metainfo(const char* bencoded_value, unsigned long length);

void free_metainfo(metainfo_t* metainfo);
//Returns magnet_data struct of parsed magnet link
magnet_data* process_magnet(const char* magnet);

void free_magnet_data(magnet_data* magnet_data);
#endif //WHOLE_BENCODE_H
