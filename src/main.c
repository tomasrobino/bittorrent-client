#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//Check if char is a digit
bool is_digit(char c);

//Decode bencoded string, returns decoded string
char* decode_bencode(const char* bencoded_value);

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


bool is_digit(const char c) {
    return c >= '0' && c <= '9';
}


char* decode_bencode(const char* bencoded_value) {
    // Byte strings
    if (is_digit(bencoded_value[0])) {
        const int length = atoi(bencoded_value);
        const char* colon_index = strchr(bencoded_value, ':');
        if (colon_index != NULL) {
            const char* start = colon_index + 1;
            char* decoded_str = malloc(length + 3);
            decoded_str[0] = '"';
            strncpy(decoded_str+1, start, length);
            decoded_str[length+1] = '"';
            decoded_str[length+2] = '\0';
            return decoded_str;
        }
        fprintf(stderr, "Invalid encoded value: %s\n", bencoded_value);
        exit(1);
    }

    fprintf(stderr, "Unsupported formatting\n");
    exit(1);
}