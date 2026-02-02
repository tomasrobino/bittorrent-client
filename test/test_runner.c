#include "unity.h"
#include "test_util.h"
#include "test_file.h"
#include "test_messages.h"

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

    // messages tests

    // bitfield_to_hex tests
    RUN_TEST(test_bitfield_to_hex_normal);
    RUN_TEST(test_bitfield_to_hex_all_zero);
    RUN_TEST(test_bitfield_to_hex_all_ff);
    RUN_TEST(test_bitfield_to_hex_single_byte);
    RUN_TEST(test_bitfield_to_hex_zero_length);

    // check_handshake tests
    RUN_TEST(test_check_handshake_valid);
    RUN_TEST(test_check_handshake_wrong_protocol);
    RUN_TEST(test_check_handshake_wrong_info_hash);

    // process_bitfield tests
    RUN_TEST(test_process_bitfield_identical);
    RUN_TEST(test_process_bitfield_client_empty);
    RUN_TEST(test_process_bitfield_foreign_empty);
    RUN_TEST(test_process_bitfield_zero_size);

    // read_message_length tests
    RUN_TEST(test_read_message_length_keep_alive);
    RUN_TEST(test_read_message_length_normal);

    // handle_have tests
    RUN_TEST(test_handle_have_allocates_bitfield);
    RUN_TEST(test_handle_have_peer_already_has_piece);
    RUN_TEST(test_handle_have_client_has_piece);
    RUN_TEST(test_handle_have_client_missing_piece);
    RUN_TEST(test_handle_have_second_byte_piece);
    RUN_TEST(test_handle_have_piece_index_zero);
    RUN_TEST(test_handle_have_last_bit);
    RUN_TEST(test_handle_have_invalid_index);

    // handle_bitfield tests
    RUN_TEST(test_handle_bitfield_null_payload);

    // write_block tests
    RUN_TEST(test_write_block_normal);
    RUN_TEST(test_write_block_zero);
    RUN_TEST(test_write_block_null_file);

    return UNITY_END();
}
