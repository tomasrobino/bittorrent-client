#ifndef PARSING_H
#define PARSING_H
#include "structs.h"

// For decoding announce-list
announce_list_ll* decode_announce_list(const char* announce_list, unsigned long* index);

files_ll* read_info_files(const char* bencode, bool multiple, unsigned long* index);

metainfo_t* parse_metainfo(const char* bencoded_value, unsigned long length);
//Returns magnet_data struct of parsed magnet link
magnet_data* process_magnet(const char* magnet);

void free_magnet_data(magnet_data* magnet_data);
#endif //PARSING_H
