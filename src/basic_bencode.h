#ifndef BASIC_BENCODE_H
#define BASIC_BENCODE_H
#include "structs.h"

//Check if char is a digit
bool is_digit(char c);
// Decode bencoded list, returns linked list with elements
ll* decode_bencode_list(const char* bencoded_list, unsigned int* length);
//Decode bencoded string, returns decoded string
char* decode_bencode_string(const char* bencoded_value);

int decode_bencode_int(const char* bencoded_value, char** endptr);

#endif //BASIC_BENCODE_H
