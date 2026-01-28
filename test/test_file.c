#include <stdlib.h>
#include <string.h>

#include "unity.h"
#include "../src/file.h"


void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

void test_sha1_to_hex(void) {
    const unsigned char sha1[20] = {
        0x12,0x34,0x56,0x78,0x9a,0xbc,0xde,0xf0,0x11,0x22,
        0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb,0xcc
    };
    char hex[41] = {0};
    sha1_to_hex(sha1, hex);

    TEST_ASSERT_EQUAL_STRING("123456789abcdef0112233445566778899aabbcc", hex);
}

void test_sha1_to_hex_all_zeros(void) {
    unsigned char sha1[20] = {0};
    char hex[41] = {0};
    sha1_to_hex(sha1, hex);
    TEST_ASSERT_EQUAL_STRING("0000000000000000000000000000000000000000", hex);
}

void test_sha1_to_hex_all_ff(void) {
    unsigned char sha1[20];
    memset(sha1, 0xFF, 20);
    char hex[41] = {0};
    sha1_to_hex(sha1, hex);
    TEST_ASSERT_EQUAL_STRING("ffffffffffffffffffffffffffffffffffffffff", hex);
}


void test_free_announce_list(void) {
    announce_list_ll *node1 = malloc(sizeof(announce_list_ll));
    announce_list_ll *node2 = malloc(sizeof(announce_list_ll));
    node1->next = node2;
    node1->list = nullptr;
    node2->next = nullptr;
    node2->list = nullptr;

    free_announce_list(node1);

    // Can't check freed memory directly, but we can test with tools like Valgrind for leaks
    TEST_PASS();
}

void test_free_announce_list_null(void) {
    free_announce_list(nullptr);
    TEST_PASS(); // Should not crash
}

void test_free_announce_list_single_node(void) {
    announce_list_ll *node = malloc(sizeof(announce_list_ll));
    node->list = nullptr;
    node->next = nullptr;

    free_announce_list(node);
    TEST_PASS(); // No crash
}


void test_parse_metainfo(void) {
    const char *bencoded = "d4:infod6:lengthi12345e4:name9:testfilee"; // minimal torrent
    metainfo_t *info = parse_metainfo(bencoded, strlen(bencoded), LOG_NO);

    TEST_ASSERT_NOT_NULL(info);
    TEST_ASSERT_EQUAL(12345, info->info->length);
    TEST_ASSERT_EQUAL_STRING("testfile", info->info->name);

    free_metainfo(info);
}

void test_parse_metainfo_empty_string(void) {
    metainfo_t *info = parse_metainfo("", 0, LOG_NO);
    TEST_ASSERT_NULL(info);
}

void test_parse_metainfo_invalid_bencode(void) {
    const char *invalid = "this_is_not_bencode";
    metainfo_t *info = parse_metainfo(invalid, strlen(invalid), LOG_NO);
    TEST_ASSERT_NULL(info);
}

void test_parse_metainfo_missing_name(void) {
    const char *bencoded = "d4:infod6:lengthi100e5:piecee"; // no name
    metainfo_t *info = parse_metainfo(bencoded, strlen(bencoded), LOG_NO);
    TEST_ASSERT_NULL(info); // or check parser-defined behavior
}


void test_free_info_files_list(void) {
    files_ll *file = malloc(sizeof(files_ll));
    file->path = nullptr;
    file->next = nullptr;

    free_info_files_list(file);
    TEST_PASS(); // Manual inspection with Valgrind
}

void test_free_info_files_list_null(void) {
    free_info_files_list(nullptr);
    TEST_PASS();
}

void test_free_info_files_list_nested_path(void) {
    files_ll *file = malloc(sizeof(files_ll));
    ll *path_node = malloc(sizeof(ll));
    path_node->next = nullptr;
    file->path = path_node;
    file->next = nullptr;
    file->file_ptr = nullptr;

    free_info_files_list(file);
    TEST_PASS(); // Check with Valgrind
}


void test_free_metainfo(void) {
    metainfo_t *meta = malloc(sizeof(metainfo_t));
    meta->announce = strdup("tracker");
    meta->info = malloc(sizeof(info_t));
    meta->info->files = nullptr;

    free_metainfo(meta);
    TEST_PASS(); // Again, check memory leaks with Valgrind
}

void test_free_metainfo_all_null(void) {
    metainfo_t meta = {0};
    free_metainfo(&meta); // Should not crash
    TEST_PASS();
}

void test_free_metainfo_with_files(void) {
    files_ll *file = malloc(sizeof(files_ll));
    file->path = nullptr;
    file->next = nullptr;

    info_t *info = malloc(sizeof(info_t));
    info->files = file;
    info->name = strdup("testfile");

    metainfo_t *meta = malloc(sizeof(metainfo_t));
    meta->info = info;
    meta->announce = strdup("tracker");

    free_metainfo(meta);
    TEST_PASS(); // Check with Valgrind
}

void test_info_t_zero_lengths(void) {
    info_t info = {0};
    info.length = 0;
    info.piece_length = 0;
    info.piece_number = 0;
    TEST_ASSERT_EQUAL(0, info.length);
    TEST_ASSERT_EQUAL(0, info.piece_length);
    TEST_ASSERT_EQUAL(0, info.piece_number);
}


int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_sha1_to_hex);
    RUN_TEST(test_free_announce_list);
    RUN_TEST(test_parse_metainfo);
    RUN_TEST(test_free_info_files_list);
    RUN_TEST(test_free_metainfo);

    return UNITY_END();
}
