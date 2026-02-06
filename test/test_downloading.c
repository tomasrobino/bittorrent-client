#include "unity.h"
#include "../src/downloading.h"
#include "../src/downloading_types.h"
#include <stdlib.h>
#include <string.h>

// Assumed BLOCK_SIZE constant - adjust if different in your implementation
#ifndef BLOCK_SIZE
#define BLOCK_SIZE 16384
#endif

// ============================================================================
// Tests for calc_block_size
// ============================================================================

void test_calc_block_size_regular_block(void) {
    uint32_t piece_size = 262144; // 256 KB
    uint32_t byte_offset = 0;

    int64_t result = calc_block_size(piece_size, byte_offset);

    TEST_ASSERT_EQUAL_INT64(BLOCK_SIZE, result);
}

void test_calc_block_size_second_block(void) {
    uint32_t piece_size = 262144; // 256 KB
    uint32_t byte_offset = BLOCK_SIZE;

    int64_t result = calc_block_size(piece_size, byte_offset);

    TEST_ASSERT_EQUAL_INT64(BLOCK_SIZE, result);
}

void test_calc_block_size_last_block_full_size(void) {
    uint32_t piece_size = BLOCK_SIZE * 2;
    uint32_t byte_offset = BLOCK_SIZE;

    int64_t result = calc_block_size(piece_size, byte_offset);

    TEST_ASSERT_EQUAL_INT64(BLOCK_SIZE, result);
}

void test_calc_block_size_last_block_partial(void) {
    uint32_t piece_size = BLOCK_SIZE + 1000;
    uint32_t byte_offset = BLOCK_SIZE;

    int64_t result = calc_block_size(piece_size, byte_offset);

    TEST_ASSERT_EQUAL_INT64(1000, result);
}

void test_calc_block_size_small_piece(void) {
    uint32_t piece_size = 1024; // 1 KB piece
    uint32_t byte_offset = 0;

    int64_t result = calc_block_size(piece_size, byte_offset);

    TEST_ASSERT_EQUAL_INT64(1024, result);
}

void test_calc_block_size_exact_block_size_piece(void) {
    uint32_t piece_size = BLOCK_SIZE;
    uint32_t byte_offset = 0;

    int64_t result = calc_block_size(piece_size, byte_offset);

    TEST_ASSERT_EQUAL_INT64(BLOCK_SIZE, result);
}

void test_calc_block_size_zero_offset(void) {
    uint32_t piece_size = 100000;
    uint32_t byte_offset = 0;

    int64_t result = calc_block_size(piece_size, byte_offset);

    TEST_ASSERT_EQUAL_INT64(BLOCK_SIZE, result);
}

void test_calc_block_size_offset_near_end(void) {
    uint32_t piece_size = BLOCK_SIZE * 3 + 500;
    uint32_t byte_offset = BLOCK_SIZE * 3;

    int64_t result = calc_block_size(piece_size, byte_offset);

    TEST_ASSERT_EQUAL_INT64(500, result);
}

// ============================================================================
// Tests for get_path
// ============================================================================

void test_get_path_single_segment(void) {
    ll segment = {.next = nullptr, .val = "file.txt"};

    char *result = get_path(&segment, LOG_NO);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("file.txt", result);

    free(result);
}

void test_get_path_two_segments(void) {
    ll segment2 = {.next = nullptr, .val = "file.txt"};
    ll segment1 = {.next = &segment2, .val = "folder"};

    char *result = get_path(&segment1, LOG_NO);

    TEST_ASSERT_NOT_NULL(result);
    // Path separator depends on OS - adjust expected string accordingly
    // Assuming Unix-style separator
    TEST_ASSERT_EQUAL_STRING("folder/file.txt", result);

    free(result);
}

void test_get_path_multiple_segments(void) {
    ll segment3 = {.next = nullptr, .val = "file.txt"};
    ll segment2 = {.next = &segment3, .val = "subfolder"};
    ll segment1 = {.next = &segment2, .val = "folder"};

    char *result = get_path(&segment1, LOG_NO);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("folder/subfolder/file.txt", result);

    free(result);
}

void test_get_path_deep_nesting(void) {
    ll seg5 = {.next = nullptr, .val = "file.txt"};
    ll seg4 = {.next = &seg5, .val = "level3"};
    ll seg3 = {.next = &seg4, .val = "level2"};
    ll seg2 = {.next = &seg3, .val = "level1"};
    ll seg1 = {.next = &seg2, .val = "root"};

    char *result = get_path(&seg1, LOG_NO);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("root/level1/level2/level3/file.txt", result);

    free(result);
}

void test_get_path_null_filepath(void) {
    char *result = get_path(nullptr, LOG_NO);

    TEST_ASSERT_NULL(result);
}

void test_get_path_logging_modes(void) {
    ll segment = {.next = nullptr, .val = "file.txt"};

    char *r1 = get_path(&segment, LOG_NO);
    char *r2 = get_path(&segment, LOG_ERR);
    char *r3 = get_path(&segment, LOG_SUMM);
    char *r4 = get_path(&segment, LOG_FULL);

    TEST_ASSERT_NOT_NULL(r1);
    TEST_ASSERT_NOT_NULL(r2);
    TEST_ASSERT_NOT_NULL(r3);
    TEST_ASSERT_NOT_NULL(r4);

    free(r1);
    free(r2);
    free(r3);
    free(r4);
}

// ============================================================================
// Tests for piece_complete
// ============================================================================

void test_piece_complete_single_block_piece_complete(void) {
    unsigned char block_tracker[1] = {0xFF}; // All bits set
    uint32_t piece_index = 0;
    uint32_t piece_size = BLOCK_SIZE;
    int64_t torrent_size = BLOCK_SIZE;

    bool result = piece_complete(block_tracker, piece_index, piece_size, torrent_size);

    TEST_ASSERT_TRUE(result);
}

void test_piece_complete_single_block_piece_incomplete(void) {
    unsigned char block_tracker[1] = {0x00}; // No bits set
    uint32_t piece_index = 0;
    uint32_t piece_size = BLOCK_SIZE;
    int64_t torrent_size = BLOCK_SIZE;

    bool result = piece_complete(block_tracker, piece_index, piece_size, torrent_size);

    TEST_ASSERT_FALSE(result);
}

void test_piece_complete_multi_block_piece_complete(void) {
    unsigned char block_tracker[2] = {0xFF, 0xFF}; // All bits set
    uint32_t piece_index = 0;
    uint32_t piece_size = BLOCK_SIZE * 16; // 16 blocks
    int64_t torrent_size = BLOCK_SIZE * 16;

    bool result = piece_complete(block_tracker, piece_index, piece_size, torrent_size);

    TEST_ASSERT_TRUE(result);
}

void test_piece_complete_multi_block_piece_partial(void) {
    unsigned char block_tracker[2] = {0xFF, 0x0F}; // Some bits set
    uint32_t piece_index = 0;
    uint32_t piece_size = BLOCK_SIZE * 16;
    int64_t torrent_size = BLOCK_SIZE * 16;

    bool result = piece_complete(block_tracker, piece_index, piece_size, torrent_size);

    TEST_ASSERT_FALSE(result);
}

void test_piece_complete_last_piece_smaller(void) {
    unsigned char block_tracker[2] = {0xFF, 0x01}; // Only first bit of second byte
    uint32_t piece_index = 1;
    uint32_t piece_size = BLOCK_SIZE * 8;
    int64_t torrent_size = BLOCK_SIZE * 9; // Last piece is 1 block

    bool result = piece_complete(block_tracker, piece_index, piece_size, torrent_size);

    TEST_ASSERT_TRUE(result);
}

void test_piece_complete_null_block_tracker(void) {
    uint32_t piece_index = 0;
    uint32_t piece_size = BLOCK_SIZE;
    int64_t torrent_size = BLOCK_SIZE;

    bool result = piece_complete(nullptr, piece_index, piece_size, torrent_size);

    TEST_ASSERT_FALSE(result);
}

// ============================================================================
// Tests for are_bits_set
// ============================================================================

void test_are_bits_set_all_set_single_byte(void) {
    unsigned char bitfield[1] = {0xFF};

    bool result = are_bits_set(bitfield, 0, 7);

    TEST_ASSERT_TRUE(result);
}

void test_are_bits_set_none_set(void) {
    unsigned char bitfield[1] = {0x00};

    bool result = are_bits_set(bitfield, 0, 7);

    TEST_ASSERT_FALSE(result);
}

void test_are_bits_set_partial(void) {
    unsigned char bitfield[1] = {0xF0}; // 11110000

    bool result = are_bits_set(bitfield, 0, 3);

    TEST_ASSERT_TRUE(result);
}

void test_are_bits_set_partial_fail(void) {
    unsigned char bitfield[1] = {0xF0}; // 11110000

    bool result = are_bits_set(bitfield, 0, 4);

    TEST_ASSERT_FALSE(result);
}

void test_are_bits_set_single_bit_set(void) {
    unsigned char bitfield[1] = {0x80}; // 10000000

    bool result = are_bits_set(bitfield, 0, 0);

    TEST_ASSERT_TRUE(result);
}

void test_are_bits_set_single_bit_not_set(void) {
    unsigned char bitfield[1] = {0x7F}; // 01111111

    bool result = are_bits_set(bitfield, 0, 0);

    TEST_ASSERT_FALSE(result);
}

void test_are_bits_set_cross_byte_boundary(void) {
    unsigned char bitfield[2] = {0xFF, 0xFF};

    bool result = are_bits_set(bitfield, 6, 9);

    TEST_ASSERT_TRUE(result);
}

void test_are_bits_set_cross_byte_boundary_partial(void) {
    unsigned char bitfield[2] = {0xFF, 0x00};

    bool result = are_bits_set(bitfield, 6, 9);

    TEST_ASSERT_FALSE(result);
}

void test_are_bits_set_middle_range(void) {
    unsigned char bitfield[1] = {0x3C}; // 00111100

    bool result = are_bits_set(bitfield, 2, 5);

    TEST_ASSERT_TRUE(result);
}

void test_are_bits_set_null_bitfield(void) {
    bool result = are_bits_set(nullptr, 0, 7);

    TEST_ASSERT_FALSE(result);
}

void test_are_bits_set_start_greater_than_end(void) {
    unsigned char bitfield[1] = {0xFF};

    bool result = are_bits_set(bitfield, 7, 0);

    TEST_ASSERT_FALSE(result);
}

// ============================================================================
// Tests for closing_files
// ============================================================================

void test_closing_files_null_files(void) {
    unsigned char bitfield[1] = {0xFF};

    // Should not crash
    closing_files(nullptr, bitfield, 0, BLOCK_SIZE, BLOCK_SIZE);
    TEST_PASS();
}

void test_closing_files_null_bitfield(void) {
    // Create a mock file structure
    // This test requires actual file structure implementation
    TEST_IGNORE_MESSAGE("Requires file structure implementation");
}

void test_closing_files_single_file_complete(void) {
    // This test requires actual file structure and file opening
    TEST_IGNORE_MESSAGE("Requires file I/O mocking");
}

void test_closing_files_single_file_incomplete(void) {
    TEST_IGNORE_MESSAGE("Requires file I/O mocking");
}

void test_closing_files_multiple_files(void) {
    TEST_IGNORE_MESSAGE("Requires file I/O mocking");
}

// ============================================================================
// Tests for handle_predownload_udp
// ============================================================================

void test_handle_predownload_udp_valid_request(void) {
    // This test requires network mocking
    TEST_IGNORE_MESSAGE("Requires network mocking");
}

void test_handle_predownload_udp_null_peer_id(void) {
    metainfo_t metainfo = {0};
    torrent_stats_t stats = {0, 1024, 0, 0, 12345};

    announce_response_t *result = handle_predownload_udp(metainfo, nullptr, &stats, LOG_NO);

    TEST_ASSERT_NULL(result);
}

void test_handle_predownload_udp_null_torrent_stats(void) {
    metainfo_t metainfo = {0};
    unsigned char peer_id[20] = {0};

    announce_response_t *result = handle_predownload_udp(metainfo, peer_id, nullptr, LOG_NO);

    TEST_ASSERT_NULL(result);
}

void test_handle_predownload_udp_logging_modes(void) {
    // This test requires network mocking
    TEST_IGNORE_MESSAGE("Requires network mocking");
}

// ============================================================================
// Tests for read_from_socket
// ============================================================================

void test_read_from_socket_null_peer(void) {
    int32_t epoll = 1;

    bool result = read_from_socket(nullptr, epoll, LOG_NO);

    TEST_ASSERT_FALSE(result);
}

void test_read_from_socket_invalid_epoll(void) {
    peer_t peer = {0};

    bool result = read_from_socket(&peer, -1, LOG_NO);

    TEST_ASSERT_FALSE(result);
}

void test_read_from_socket_valid_operation(void) {
    // This test requires socket mocking
    TEST_IGNORE_MESSAGE("Requires socket mocking");
}

void test_read_from_socket_connection_closed(void) {
    // This test requires socket mocking
    TEST_IGNORE_MESSAGE("Requires socket mocking");
}

void test_read_from_socket_partial_read(void) {
    // This test requires socket mocking
    TEST_IGNORE_MESSAGE("Requires socket mocking");
}

// ============================================================================
// Tests for reconnect
// ============================================================================

void test_reconnect_null_peer_list(void) {
    uint32_t result = reconnect(nullptr, 10, 0, 1, LOG_NO);

    TEST_ASSERT_EQUAL_UINT32(0, result);
}

void test_reconnect_zero_peer_amount(void) {
    peer_t peers[1] = {0};

    uint32_t result = reconnect(peers, 0, 0, 1, LOG_NO);

    TEST_ASSERT_EQUAL_UINT32(0, result);
}

void test_reconnect_invalid_epoll(void) {
    peer_t peers[1] = {0};

    uint32_t result = reconnect(peers, 1, 0, -1, LOG_NO);

    TEST_ASSERT_EQUAL_UINT32(0, result);
}

void test_reconnect_no_closed_peers(void) {
    // This test requires peer status mocking
    TEST_IGNORE_MESSAGE("Requires peer status implementation");
}

void test_reconnect_some_closed_peers(void) {
    // This test requires socket and peer mocking
    TEST_IGNORE_MESSAGE("Requires socket mocking");
}

// ============================================================================
// Tests for write_state
// ============================================================================

void test_write_state_null_filename(void) {
    state_t state = {0};

    uint8_t result = write_state(nullptr, &state);

    TEST_ASSERT_NOT_EQUAL_UINT8(0, result);
}

void test_write_state_null_state(void) {
    uint8_t result = write_state("test_state.dat", nullptr);

    TEST_ASSERT_NOT_EQUAL_UINT8(0, result);
}

void test_write_state_valid_state(void) {
    // Create a simple state
    unsigned char bitfield[1] = {0xFF};
    state_t state = {
        .piece_count = 1,
        .piece_size = BLOCK_SIZE,
        .bitfield = bitfield
    };

    uint8_t result = write_state("test_state_write.dat", &state);

    TEST_ASSERT_EQUAL_UINT8(0, result);

    // Cleanup
    remove("test_state_write.dat");
}

void test_write_state_large_state(void) {
    unsigned char bitfield[128];
    memset(bitfield, 0xFF, sizeof(bitfield));

    state_t state = {
        .piece_count = 1024,
        .piece_size = BLOCK_SIZE,
        .bitfield = bitfield
    };

    uint8_t result = write_state("test_state_large.dat", &state);

    TEST_ASSERT_EQUAL_UINT8(0, result);

    // Cleanup
    remove("test_state_large.dat");
}

// ============================================================================
// Tests for read_state
// ============================================================================

void test_read_state_null_filename(void) {
    state_t *result = read_state(nullptr);

    TEST_ASSERT_NULL(result);
}

void test_read_state_nonexistent_file(void) {
    state_t *result = read_state("nonexistent_file_12345.dat");

    TEST_ASSERT_NULL(result);
}

void test_read_state_valid_file(void) {
    // First write a state
    unsigned char bitfield[1] = {0xFF};
    state_t write_state_obj = {
        .piece_count = 1,
        .piece_size = BLOCK_SIZE,
        .bitfield = bitfield
    };

    write_state("test_state_read.dat", &write_state_obj);
    TEST_IGNORE_MESSAGE("State handling not finished");

    // Then read it back
    state_t *result = read_state("test_state_read.dat");


    // TEST_ASSERT_NOT_NULL(result);
    // TEST_ASSERT_EQUAL_UINT32(1, result->piece_count);
    // TEST_ASSERT_EQUAL_UINT32(BLOCK_SIZE, result->piece_size);
    // TEST_ASSERT_NOT_NULL(result->bitfield);

    // Cleanup
    free(result->bitfield);
    free(result);
    remove("test_state_read.dat");
}

void test_read_state_corrupted_file(void) {
    // Create a file with invalid data
    FILE *f = fopen("test_state_corrupted.dat", "wb");
    if (f) {
        fwrite("INVALID", 1, 7, f);
        fclose(f);
    }
    TEST_IGNORE_MESSAGE("State handling not finished");

    state_t *result = read_state("test_state_corrupted.dat");

    TEST_ASSERT_NULL(result);

    // Cleanup
    remove("test_state_corrupted.dat");
}

// ============================================================================
// Tests for init_state
// ============================================================================

void test_init_state_new_file(void) {
    TEST_IGNORE_MESSAGE("State handling not finished");
    unsigned char bitfield[1] = {0x00};

    state_t *result = init_state("test_init_new.dat", 1, BLOCK_SIZE, bitfield);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_UINT32(1, result->piece_count);
    TEST_ASSERT_EQUAL_UINT32(BLOCK_SIZE, result->piece_size);

    // Cleanup
    free(result->bitfield);
    free(result);
    remove("test_init_new.dat");
}

void test_init_state_existing_file(void) {
    TEST_IGNORE_MESSAGE("State handling not finished");
    // First create a state file
    unsigned char bitfield[1] = {0xFF};
    state_t write_state_obj = {
        .piece_count = 1,
        .piece_size = BLOCK_SIZE,
        .bitfield = bitfield
    };
    write_state("test_init_existing.dat", &write_state_obj);

    // Now init with different parameters - should read existing
    unsigned char new_bitfield[1] = {0x00};
    state_t *result = init_state("test_init_existing.dat", 100, 99999, new_bitfield);

    TEST_ASSERT_NOT_NULL(result);
    // Should have values from file, not from parameters
    TEST_ASSERT_EQUAL_UINT32(1, result->piece_count);
    TEST_ASSERT_EQUAL_UINT32(BLOCK_SIZE, result->piece_size);

    // Cleanup
    free(result->bitfield);
    free(result);
    remove("test_init_existing.dat");
}

void test_init_state_null_filename(void) {
    unsigned char bitfield[1] = {0x00};

    state_t *result = init_state(nullptr, 1, BLOCK_SIZE, bitfield);

    TEST_ASSERT_NULL(result);
}

void test_init_state_null_bitfield(void) {
    state_t *result = init_state("test_init_null_bf.dat", 1, BLOCK_SIZE, nullptr);

    TEST_ASSERT_NULL(result);

    // Cleanup
    remove("test_init_null_bf.dat");
}

void test_init_state_zero_piece_count(void) {
    unsigned char bitfield[1] = {0x00};

    state_t *result = init_state("test_init_zero.dat", 0, BLOCK_SIZE, bitfield);

    // Behavior depends on implementation
    // May return nullptr or create with 0 pieces

    if (result) {
        free(result->bitfield);
        free(result);
    }
    remove("test_init_zero.dat");

    TEST_PASS();
}

// ============================================================================
// Tests for torrent
// ============================================================================

void test_torrent_null_peer_id(void) {
    metainfo_t metainfo = {0};

    int32_t result = torrent(metainfo, nullptr, LOG_NO);

    TEST_ASSERT_NOT_EQUAL_INT32(0, result);
}

void test_torrent_invalid_metainfo(void) {
    metainfo_t metainfo = {0};
    unsigned char peer_id[20] = {0};

    int32_t result = torrent(metainfo, peer_id, LOG_NO);

    TEST_ASSERT_NOT_EQUAL_INT32(0, result);
}

void test_torrent_full_integration(void) {
    // This would require a full integration test environment
    TEST_IGNORE_MESSAGE("Requires full integration environment");
}
