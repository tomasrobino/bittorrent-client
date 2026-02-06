#include <stdlib.h>
#include <string.h>

#include "unity.h"
#include "../src/messages.h"

// bitfield_to_hex()

void test_bitfield_to_hex_normal(void) {
    const unsigned char bf[] = {0x12, 0xAB, 0x00};
    char hex[7] = {0};
    bitfield_to_hex(bf, 3, hex);
    TEST_ASSERT_EQUAL_STRING("12ab00", hex);
}

void test_bitfield_to_hex_all_zero(void) {
    const unsigned char bf[] = {0x00, 0x00};
    char hex[5] = {0};
    bitfield_to_hex(bf, 2, hex);
    TEST_ASSERT_EQUAL_STRING("0000", hex);
}

void test_bitfield_to_hex_all_ff(void) {
    const unsigned char bf[] = {0xFF, 0xFF};
    char hex[5] = {0};
    bitfield_to_hex(bf, 2, hex);
    TEST_ASSERT_EQUAL_STRING("ffff", hex);
}

void test_bitfield_to_hex_single_byte(void) {
    const unsigned char bf[] = {0x5A};
    char hex[3] = {0};
    bitfield_to_hex(bf, 1, hex);
    TEST_ASSERT_EQUAL_STRING("5a", hex);
}

void test_bitfield_to_hex_zero_length(void) {
    char hex[1] = {0};
    bitfield_to_hex(nullptr, 0, hex);
    TEST_ASSERT_EQUAL_STRING("", hex);
}

// check_handshake()

void test_check_handshake_valid(void) {
    unsigned char info_hash[20] = {1,2,3};
    unsigned char buffer[68] = {0};

    buffer[0] = 19;
    memcpy(&buffer[1], "BitTorrent protocol", 19);
    memcpy(&buffer[28], info_hash, 20);

    TEST_ASSERT_TRUE(check_handshake(info_hash, buffer));
}

void test_check_handshake_wrong_protocol(void) {
    unsigned char info_hash[20] = {1};
    unsigned char buffer[68] = {0};

    buffer[0] = 19;
    memcpy(&buffer[1], "BadTorrent protocol", 19);
    memcpy(&buffer[28], info_hash, 20);

    TEST_ASSERT_FALSE(check_handshake(info_hash, buffer));
}

void test_check_handshake_wrong_info_hash(void) {
    unsigned char info_hash[20] = {1};
    unsigned char other_hash[20] = {2};
    unsigned char buffer[68] = {0};

    buffer[0] = 19;
    memcpy(&buffer[1], "BitTorrent protocol", 19);
    memcpy(&buffer[28], other_hash, 20);

    TEST_ASSERT_FALSE(check_handshake(info_hash, buffer));
}

// precess_bitfield()

void test_process_bitfield_identical(void) {
    unsigned char c[] = {0xFF};
    unsigned char f[] = {0xFF};

    unsigned char *r = process_bitfield(c, f, 8);
    TEST_ASSERT_EQUAL_UINT8(0x00, r[0]);
    free(r);
}

void test_process_bitfield_client_empty(void) {
    unsigned char c[] = {0x00};
    unsigned char f[] = {0xF0};

    unsigned char *r = process_bitfield(c, f, 8);
    TEST_ASSERT_EQUAL_UINT8(0xF0, r[0]);
    free(r);
}

void test_process_bitfield_foreign_empty(void) {
    unsigned char c[] = {0xFF};
    unsigned char f[] = {0x00};

    unsigned char *r = process_bitfield(c, f, 8);
    TEST_ASSERT_EQUAL_UINT8(0x00, r[0]);
    free(r);
}

void test_process_bitfield_zero_size(void) {
    unsigned char *r = process_bitfield(nullptr, nullptr, 0);
    TEST_ASSERT_NULL(r);
}

// read_message_length()

void test_read_message_length_keep_alive(void) {
    unsigned char* buffer = malloc(4);
    time_t t = 0;
    memset(buffer, 0, 4);

    TEST_ASSERT_FALSE(read_message_length(buffer, &t));
    TEST_ASSERT_NOT_EQUAL(0, t);
}

void test_read_message_length_normal(void) {
    unsigned char buffer[4] = {0,0,0,5};
    time_t t = 0;

    TEST_ASSERT_TRUE(read_message_length(buffer, &t));
    TEST_ASSERT_NOT_EQUAL(0, t);
}

// handle_have()

void test_handle_have_allocates_bitfield(void) {
    peer_t peer = {0};
    unsigned char payload[4] = {0,0,0,1}; // piece index 1
    unsigned char client_bf[1] = {0x00};

    handle_have(&peer, payload, client_bf, 1, LOG_NO);

    TEST_ASSERT_NOT_NULL(peer.bitfield);
    free(peer.bitfield);
}

// Peer already has the piece → no change, am_interested false
void test_handle_have_peer_already_has_piece(void) {
    peer_t peer = {0};
    peer.bitfield = malloc(1);
    peer.bitfield[0] = 0x20; // piece 2 already set
    peer.am_interested = false;

    unsigned char payload[4] = {0,0,0,2}; // piece index 2
    unsigned char client_bf[1] = {0x20};

    handle_have(&peer, payload, client_bf, 1, LOG_NO);

    TEST_ASSERT_EQUAL_UINT8(0x20, peer.bitfield[0]);
    TEST_ASSERT_FALSE(peer.am_interested);
    free(peer.bitfield);
}

// Peer announces piece client already has → am_interested stays false
void test_handle_have_client_has_piece(void) {
    peer_t peer = {0};
    peer.bitfield = malloc(1);
    peer.bitfield[0] = 0x00;
    peer.am_interested = false;

    unsigned char payload[4] = {0,0,0,1}; // piece 1
    unsigned char client_bf[1] = {0x02}; // client already has piece 1

    handle_have(&peer, payload, client_bf, 1, LOG_NO);

    TEST_ASSERT_FALSE(peer.am_interested);
    TEST_ASSERT_EQUAL_UINT8(0x02, peer.bitfield[0] & 0x02); // bitfield updated
    free(peer.bitfield);
}

// Peer announces piece client doesn’t have → am_interested becomes true
void test_handle_have_client_missing_piece(void) {
    peer_t peer = {0};
    peer.bitfield = malloc(1);
    peer.bitfield[0] = 0x00;
    peer.am_interested = false;

    unsigned char payload[4] = {0,0,0,0}; // piece 0
    unsigned char client_bf[1] = {0x00}; // client missing it

    handle_have(&peer, payload, client_bf, 1, LOG_NO);

    TEST_ASSERT_TRUE(peer.am_interested);
    free(peer.bitfield);
}

// Multi-byte bitfield, piece in second byte
void test_handle_have_second_byte_piece(void) {
    peer_t peer = {0};
    peer.bitfield = malloc(2);
    memset(peer.bitfield, 0, 2);
    peer.am_interested = false;

    unsigned char payload[4] = {0,0,0,9}; // piece index 9 → second byte
    unsigned char client_bf[2] = {0xFF, 0x00}; // client missing piece 9

    handle_have(&peer, payload, client_bf, 2, LOG_NO);

    TEST_ASSERT_TRUE(peer.am_interested);
    free(peer.bitfield);
}

// Edge: piece index = 0 (first bit of first byte)
void test_handle_have_piece_index_zero(void) {
    peer_t peer = {0};
    peer.bitfield = malloc(1);
    peer.bitfield[0] = 0x00;
    peer.am_interested = false;

    unsigned char payload[4] = {0,0,0,0}; // piece 0
    unsigned char client_bf[1] = {0x00};

    handle_have(&peer, payload, client_bf, 1, LOG_NO);

    TEST_ASSERT_TRUE(peer.am_interested);
    free(peer.bitfield);
}

// Edge: last bit of last byte (e.g., piece 15 in 2-byte bitfield)
void test_handle_have_last_bit(void) {
    peer_t peer = {0};
    peer.bitfield = malloc(2);
    memset(peer.bitfield, 0, 2);
    peer.am_interested = false;

    unsigned char payload[4] = {0,0,0,15}; // piece 15 → last bit of second byte
    unsigned char client_bf[2] = {0xFF, 0x7F}; // client missing last bit

    handle_have(&peer, payload, client_bf, 2, LOG_NO);

    TEST_ASSERT_TRUE(peer.am_interested);
    free(peer.bitfield);
}

// handle_bitfield()

void test_handle_bitfield_null_payload(void) {
    peer_t peer = {0};
    unsigned char client_bf[1] = {0xFF};

    handle_bitfield(&peer, nullptr, client_bf, 1, LOG_NO);

    TEST_ASSERT_NOT_NULL(peer.bitfield);
}

// write_block()
// TODO: This will be enabled once block writing is working
void test_write_block_normal(void) {
    FILE *f = tmpfile();
    unsigned char data[] = {1,2,3};

    int64_t r = write_block(data, 3, f, LOG_NO);
    TEST_IGNORE_MESSAGE("This will be enabled once block writing is working");
    //TEST_ASSERT_EQUAL(3, r);

    fclose(f);
}

void test_write_block_zero(void) {
    FILE *f = tmpfile();
    unsigned char data[] = {1};
    TEST_IGNORE_MESSAGE("This will be enabled once block writing is working");
    //TEST_ASSERT_EQUAL(0, write_block(data, 0, f, LOG_NO));
    fclose(f);
}

void test_write_block_null_file(void) {
    unsigned char data[] = {1};
    TEST_IGNORE_MESSAGE("This will be enabled once block writing is working");
    //TEST_ASSERT_EQUAL(-1, write_block(data, 1, nullptr, LOG_NO));
}