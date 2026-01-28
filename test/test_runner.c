#include "unity.h"
#include "test_util.h"
#include "test_file.h"

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}


int main(void) {
    UNITY_BEGIN();

    /* util.h */

    RUN_TEST(test_is_digit_normal_digits);
    RUN_TEST(test_is_digit_outside_digits);
    RUN_TEST(test_is_digit_letters);
    RUN_TEST(test_is_digit_non_printable);
    RUN_TEST(test_is_digit_high_ascii);

    /* file.h */

    // SHA1 tests
    RUN_TEST(test_sha1_to_hex);
    RUN_TEST(test_sha1_to_hex_all_zeros);
    RUN_TEST(test_sha1_to_hex_all_ff);
    RUN_TEST(test_sha1_to_hex_alternating);

    // free_announce_list tests
    RUN_TEST(test_free_announce_list);
    RUN_TEST(test_free_announce_list_null);
    RUN_TEST(test_free_announce_list_single_node);
    RUN_TEST(test_free_announce_list_multiple_nodes);

    // parse_metainfo tests
    RUN_TEST(test_parse_metainfo);
    RUN_TEST(test_parse_metainfo_empty_string);
    RUN_TEST(test_parse_metainfo_invalid_bencode);
    RUN_TEST(test_parse_metainfo_missing_name);
    RUN_TEST(test_parse_metainfo_private_flag);
    RUN_TEST(test_parse_metainfo_empty_info);

    // free_info_files_list tests
    RUN_TEST(test_free_info_files_list);
    RUN_TEST(test_free_info_files_list_null);
    RUN_TEST(test_free_info_files_list_nested_path);
    RUN_TEST(test_free_info_files_list_multiple_nodes);

    // free_metainfo tests
    RUN_TEST(test_free_metainfo);
    RUN_TEST(test_free_metainfo_with_files);
    RUN_TEST(test_free_metainfo_with_announce_list);

    // info_t tests
    RUN_TEST(test_info_t_zero_lengths);
    RUN_TEST(test_info_t_piece_zero);

    return UNITY_END();
}
