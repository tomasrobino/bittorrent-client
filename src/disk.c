#include "disk.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "downloading_types.h"

uint8_t write_state(const char* filename, const state_t* state) {
    if (!filename || !state) return 1;

    FILE* file = fopen(filename, "wb");
    if (!file) return 1;
    uint32_t bytes_written;
    do {
        bytes_written = fwrite(&state->magic, 1, sizeof(uint32_t), file);
    } while (bytes_written != sizeof(uint32_t));
    do {
        bytes_written = fwrite(&state->version, 1, sizeof(uint8_t), file);
    } while (bytes_written != sizeof(uint8_t));
    do {
        bytes_written = fwrite(&state->piece_count, 1, sizeof(uint32_t), file);
    } while (bytes_written != sizeof(uint32_t));
    do {
        bytes_written = fwrite(&state->piece_size, 1, sizeof(uint32_t), file);
    } while (bytes_written != sizeof(uint32_t));
    do {
        bytes_written = fwrite(state->bitfield, 1, ceil(state->piece_count / 8.0), file);
    } while (bytes_written != ceil(state->piece_count / 8.0));
    fclose(file);
    return 0;
}

state_t* read_state(const char* filename) {
    errno = 0;
    // Attempt to open file
    FILE* file = fopen(filename, "rb");
    if (!file) return nullptr;

    // Actually reading file
    uint32_t total_bytes_read = 0;
    state_t* state = malloc(sizeof(state_t));
    do {
        const uint32_t bytes = fread(state, 1, STATE_T_CORE_SIZE, file);
        total_bytes_read += bytes;
    } while (total_bytes_read < STATE_T_CORE_SIZE);
    const uint32_t bitfield_byte_amount = ceil(state->piece_count / 8.0);
    state->bitfield = malloc(bitfield_byte_amount);
    do {
        const uint32_t bytes = fread(state->bitfield, 1, bitfield_byte_amount, file);
        total_bytes_read += bytes;
    } while (total_bytes_read < bitfield_byte_amount);

    return state;
}

state_t* init_state(const char* filename, const uint32_t piece_count, const uint32_t piece_size, unsigned char* bitfield) {
    if (!filename || piece_count == 0 || piece_size == 0 || !bitfield) return nullptr;

    state_t* state = read_state(filename);
    if (state != nullptr) {
        return state;
    }
    if (errno == ENOENT) {
        state = malloc(sizeof(state_t));
        memcpy(&state->magic, "BTST", 4);
        state->version = 1;
        state->piece_count = piece_count;
        state->piece_size = piece_size;
        state->bitfield = bitfield;
        return state;
    }

    while (errno != 0) {
        state = read_state(filename);
    }
    return state;
}
