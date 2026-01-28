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

void test_parse_metainfo(void) {
    const char *bencoded = "d4:infod6:lengthi12345e4:name9:testfilee"; // minimal torrent
    metainfo_t *info = parse_metainfo(bencoded, strlen(bencoded), LOG_NO);

    TEST_ASSERT_NOT_NULL(info);
    TEST_ASSERT_EQUAL(12345, info->info->length);
    TEST_ASSERT_EQUAL_STRING("testfile", info->info->name);

    free_metainfo(info);
}

void test_free_info_files_list(void) {
    files_ll *file = malloc(sizeof(files_ll));
    file->path = nullptr;
    file->next = nullptr;

    free_info_files_list(file);
    TEST_PASS(); // Manual inspection with Valgrind
}

void test_free_metainfo(void) {
    metainfo_t *meta = malloc(sizeof(metainfo_t));
    meta->announce = strdup("tracker");
    meta->info = malloc(sizeof(info_t));
    meta->info->files = nullptr;

    free_metainfo(meta);
    TEST_PASS(); // Again, check memory leaks with Valgrind
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
