#include "unity.h"
#include "test_util.h"
#include "test_file.h"
#include "test_messages.h"
#include "test_basic_bencode.h"
#include "test_parsing.h"
#include "test_predownload_udp.h"
#include "test_downloading.h"

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
    //RUN_TEST(test_parse_metainfo);
    RUN_TEST(test_parse_metainfo_empty_string);
    RUN_TEST(test_parse_metainfo_invalid_bencode);
    RUN_TEST(test_parse_metainfo_missing_name);
    //RUN_TEST(test_parse_metainfo_private_flag);
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
    RUN_TEST(test_handle_have_client_has_piece);
    RUN_TEST(test_handle_have_client_missing_piece);
    RUN_TEST(test_handle_have_second_byte_piece);
    RUN_TEST(test_handle_have_piece_index_zero);
    RUN_TEST(test_handle_have_last_bit);

    // handle_bitfield tests
    RUN_TEST(test_handle_bitfield_null_payload);

    // write_block tests
    RUN_TEST(test_write_block_normal);
    RUN_TEST(test_write_block_zero);
    RUN_TEST(test_write_block_null_file);

    /* basic_bencode.h */
    /*
    // String decoding tests
    RUN_TEST(test_decode_bencode_string_valid_simple);
    RUN_TEST(test_decode_bencode_string_valid_empty);
    RUN_TEST(test_decode_bencode_string_valid_with_spaces);
    RUN_TEST(test_decode_bencode_string_valid_with_special_chars);
    RUN_TEST(test_decode_bencode_string_valid_single_char);
    RUN_TEST(test_decode_bencode_string_valid_long_string);
    RUN_TEST(test_decode_bencode_string_valid_numbers_in_string);

    // Integer decoding tests
    RUN_TEST(test_decode_bencode_int_valid_positive);
    RUN_TEST(test_decode_bencode_int_valid_zero);
    RUN_TEST(test_decode_bencode_int_valid_large_number);
    RUN_TEST(test_decode_bencode_int_valid_with_continuation);
    RUN_TEST(test_decode_bencode_int_null_endptr);
    RUN_TEST(test_decode_bencode_int_invalid_format_no_i);
    RUN_TEST(test_decode_bencode_int_invalid_format_no_e);
    RUN_TEST(test_decode_bencode_int_invalid_not_a_number);
    RUN_TEST(test_decode_bencode_int_empty_number);

    // List decoding tests
    RUN_TEST(test_decode_bencode_list_valid_empty);
    RUN_TEST(test_decode_bencode_list_valid_single_string);
    RUN_TEST(test_decode_bencode_list_valid_single_int);
    RUN_TEST(test_decode_bencode_list_valid_multiple_strings);
    RUN_TEST(test_decode_bencode_list_valid_mixed_types);
    RUN_TEST(test_decode_bencode_list_valid_nested_list);
    RUN_TEST(test_decode_bencode_list_invalid_no_l);
    RUN_TEST(test_decode_bencode_list_invalid_no_e);
    RUN_TEST(test_decode_bencode_list_null_length_pointer);
    RUN_TEST(test_decode_bencode_list_valid_length_output);


    // Free list tests
    RUN_TEST(test_free_bencode_list_null);
    RUN_TEST(test_free_bencode_list_single_element);
    RUN_TEST(test_free_bencode_list_multiple_elements);

    // Integration tests
    RUN_TEST(test_integration_parse_complex_list);
    RUN_TEST(test_integration_logging_modes);
    */

    /* parsing.h */
    /*
    // decode_announce_list tests
    RUN_TEST(test_decode_announce_list_valid_single_tracker);
    RUN_TEST(test_decode_announce_list_valid_multiple_trackers);
    RUN_TEST(test_decode_announce_list_valid_multiple_urls_per_tier);
    RUN_TEST(test_decode_announce_list_valid_empty_list);
    RUN_TEST(test_decode_announce_list_valid_nested_empty_lists);
    RUN_TEST(test_decode_announce_list_null_index_pointer);
    RUN_TEST(test_decode_announce_list_invalid_format_no_list);
    RUN_TEST(test_decode_announce_list_invalid_format_unclosed);
    RUN_TEST(test_decode_announce_list_invalid_format_malformed_inner);
    RUN_TEST(test_decode_announce_list_valid_index_tracking);
    RUN_TEST(test_decode_announce_list_valid_udp_tracker);
    RUN_TEST(test_decode_announce_list_valid_mixed_protocols);
    RUN_TEST(test_decode_announce_list_logging_modes);

    // read_info_files single file tests
    RUN_TEST(test_read_info_files_single_file_valid_simple);
    RUN_TEST(test_read_info_files_single_file_valid_large_file);
    RUN_TEST(test_read_info_files_single_file_valid_zero_length);
    RUN_TEST(test_read_info_files_single_file_null_index);
    RUN_TEST(test_read_info_files_single_file_invalid_no_length);
    RUN_TEST(test_read_info_files_single_file_invalid_no_name);
    RUN_TEST(test_read_info_files_single_file_invalid_malformed);
    RUN_TEST(test_read_info_files_single_file_index_tracking);

    // read_info_files multiple files tests
    RUN_TEST(test_read_info_files_multiple_files_valid_two_files);
    RUN_TEST(test_read_info_files_multiple_files_valid_single_file);
    RUN_TEST(test_read_info_files_multiple_files_valid_nested_path);
    RUN_TEST(test_read_info_files_multiple_files_valid_deep_nesting);
    RUN_TEST(test_read_info_files_multiple_files_valid_empty_files_list);
    RUN_TEST(test_read_info_files_multiple_files_valid_many_files);
    RUN_TEST(test_read_info_files_multiple_files_null_index);
    RUN_TEST(test_read_info_files_multiple_files_invalid_no_files_key);
    RUN_TEST(test_read_info_files_multiple_files_invalid_no_length);
    RUN_TEST(test_read_info_files_multiple_files_invalid_no_path);
    RUN_TEST(test_read_info_files_multiple_files_invalid_malformed);
    RUN_TEST(test_read_info_files_multiple_files_index_tracking);

    // Edge case tests
    RUN_TEST(test_read_info_files_edge_case_very_long_filename);
    RUN_TEST(test_read_info_files_edge_case_special_chars_in_name);
    RUN_TEST(test_read_info_files_edge_case_max_uint64_length);

    // Logging mode tests
    RUN_TEST(test_read_info_files_logging_modes_single);
    RUN_TEST(test_read_info_files_logging_modes_multiple);

    // Integration tests
    RUN_TEST(test_integration_announce_list_complex_structure);
    RUN_TEST(test_integration_files_list_mixed_sizes);
    */
    /* predownload_udp.h */

    RUN_TEST(test_split_address_valid_udp_ipv4);
    RUN_TEST(test_split_address_valid_http_ipv4);
    RUN_TEST(test_split_address_valid_https_ipv4);
    RUN_TEST(test_split_address_valid_udp_default_port);
    RUN_TEST(test_split_address_valid_ipv4_address);
    RUN_TEST(test_split_address_valid_ipv6_address);
    RUN_TEST(test_split_address_valid_hostname_with_subdomain);
    RUN_TEST(test_split_address_valid_high_port_number);
    RUN_TEST(test_split_address_invalid_no_protocol);
    RUN_TEST(test_split_address_invalid_no_port);
    RUN_TEST(test_split_address_invalid_malformed_url);
    RUN_TEST(test_split_address_invalid_empty_string);
    RUN_TEST(test_split_address_invalid_null_pointer);

    // shuffle_address_array tests
    RUN_TEST(test_shuffle_address_array_single_element);
    RUN_TEST(test_shuffle_address_array_two_elements);
    RUN_TEST(test_shuffle_address_array_multiple_elements);
    RUN_TEST(test_shuffle_address_array_zero_length);
    RUN_TEST(test_shuffle_address_array_null_array);

    // url_to_ip tests
    RUN_TEST(test_url_to_ip_valid_ipv4_address);
    RUN_TEST(test_url_to_ip_valid_ipv6_address);
    RUN_TEST(test_url_to_ip_valid_hostname_resolution);
    RUN_TEST(test_url_to_ip_invalid_hostname);
    RUN_TEST(test_url_to_ip_null_address);
    RUN_TEST(test_url_to_ip_logging_modes);

    // try_request_udp tests
    RUN_TEST(test_try_request_udp_single_tracker_success);
    RUN_TEST(test_try_request_udp_multiple_trackers);
    RUN_TEST(test_try_request_udp_all_fail);
    RUN_TEST(test_try_request_udp_null_sockfd);
    RUN_TEST(test_try_request_udp_null_request);
    RUN_TEST(test_try_request_udp_zero_amount);

    // connect_request_udp tests
    RUN_TEST(test_connect_request_udp_valid_single_tracker);
    RUN_TEST(test_connect_request_udp_valid_multiple_trackers);
    RUN_TEST(test_connect_request_udp_null_successful_index);
    RUN_TEST(test_connect_request_udp_null_sockfd);
    RUN_TEST(test_connect_request_udp_zero_amount);

    // connect_udp tests
    RUN_TEST(test_connect_udp_valid_single_tracker);
    RUN_TEST(test_connect_udp_valid_multiple_trackers);
    RUN_TEST(test_connect_udp_null_announce_list);
    RUN_TEST(test_connect_udp_null_connection_data);
    RUN_TEST(test_connect_udp_zero_amount);

    // announce_request_udp tests
    RUN_TEST(test_announce_request_udp_valid_request);
    RUN_TEST(test_announce_request_udp_null_server_addr);
    RUN_TEST(test_announce_request_udp_null_info_hash);
    RUN_TEST(test_announce_request_udp_null_peer_id);
    RUN_TEST(test_announce_request_udp_null_torrent_stats);
    RUN_TEST(test_announce_request_udp_invalid_sockfd);
    RUN_TEST(test_announce_request_udp_zero_connection_id);

    // scrape_request_udp tests
    RUN_TEST(test_scrape_request_udp_valid_single_torrent);
    RUN_TEST(test_scrape_request_udp_valid_multiple_torrents);
    RUN_TEST(test_scrape_request_udp_null_server_addr);
    RUN_TEST(test_scrape_request_udp_null_info_hash);
    RUN_TEST(test_scrape_request_udp_invalid_sockfd);
    RUN_TEST(test_scrape_request_udp_zero_torrent_amount);

    /* downloading.h */

        // calc_block_size tests
    RUN_TEST(test_calc_block_size_regular_block);
    RUN_TEST(test_calc_block_size_second_block);
    RUN_TEST(test_calc_block_size_last_block_full_size);
    RUN_TEST(test_calc_block_size_last_block_partial);
    RUN_TEST(test_calc_block_size_small_piece);
    RUN_TEST(test_calc_block_size_exact_block_size_piece);
    RUN_TEST(test_calc_block_size_zero_offset);
    RUN_TEST(test_calc_block_size_offset_near_end);

    // get_path tests
    RUN_TEST(test_get_path_single_segment);
    RUN_TEST(test_get_path_two_segments);
    RUN_TEST(test_get_path_multiple_segments);
    RUN_TEST(test_get_path_deep_nesting);
    RUN_TEST(test_get_path_null_filepath);
    RUN_TEST(test_get_path_logging_modes);

    // piece_complete tests
    RUN_TEST(test_piece_complete_single_block_piece_complete);
    RUN_TEST(test_piece_complete_single_block_piece_incomplete);
    RUN_TEST(test_piece_complete_multi_block_piece_complete);
    RUN_TEST(test_piece_complete_multi_block_piece_partial);
    RUN_TEST(test_piece_complete_last_piece_smaller);
    RUN_TEST(test_piece_complete_null_block_tracker);

    // are_bits_set tests
    RUN_TEST(test_are_bits_set_all_set_single_byte);
    RUN_TEST(test_are_bits_set_none_set);
    RUN_TEST(test_are_bits_set_partial);
    RUN_TEST(test_are_bits_set_partial_fail);
    RUN_TEST(test_are_bits_set_single_bit_set);
    RUN_TEST(test_are_bits_set_single_bit_not_set);
    RUN_TEST(test_are_bits_set_cross_byte_boundary);
    RUN_TEST(test_are_bits_set_cross_byte_boundary_partial);
    RUN_TEST(test_are_bits_set_middle_range);
    RUN_TEST(test_are_bits_set_null_bitfield);
    RUN_TEST(test_are_bits_set_start_greater_than_end);

    // closing_files tests
    RUN_TEST(test_closing_files_null_files);
    RUN_TEST(test_closing_files_null_bitfield);
    RUN_TEST(test_closing_files_single_file_complete);
    RUN_TEST(test_closing_files_single_file_incomplete);
    RUN_TEST(test_closing_files_multiple_files);

    // handle_predownload_udp tests
    RUN_TEST(test_handle_predownload_udp_valid_request);
    RUN_TEST(test_handle_predownload_udp_null_peer_id);
    RUN_TEST(test_handle_predownload_udp_null_torrent_stats);
    RUN_TEST(test_handle_predownload_udp_logging_modes);

    // read_from_socket tests
    RUN_TEST(test_read_from_socket_null_peer);
    RUN_TEST(test_read_from_socket_invalid_epoll);
    RUN_TEST(test_read_from_socket_valid_operation);
    RUN_TEST(test_read_from_socket_connection_closed);
    RUN_TEST(test_read_from_socket_partial_read);

    // reconnect tests
    RUN_TEST(test_reconnect_null_peer_list);
    RUN_TEST(test_reconnect_zero_peer_amount);
    RUN_TEST(test_reconnect_invalid_epoll);
    RUN_TEST(test_reconnect_no_closed_peers);
    RUN_TEST(test_reconnect_some_closed_peers);

    // write_state tests
    RUN_TEST(test_write_state_null_filename);
    RUN_TEST(test_write_state_null_state);
    RUN_TEST(test_write_state_valid_state);
    RUN_TEST(test_write_state_large_state);

    // read_state tests
    RUN_TEST(test_read_state_null_filename);
    RUN_TEST(test_read_state_nonexistent_file);
    RUN_TEST(test_read_state_valid_file);
    RUN_TEST(test_read_state_corrupted_file);

    // init_state tests
    RUN_TEST(test_init_state_new_file);
    RUN_TEST(test_init_state_existing_file);
    RUN_TEST(test_init_state_null_filename);
    RUN_TEST(test_init_state_null_bitfield);
    RUN_TEST(test_init_state_zero_piece_count);

    // torrent tests
    RUN_TEST(test_torrent_null_peer_id);
    RUN_TEST(test_torrent_invalid_metainfo);
    RUN_TEST(test_torrent_full_integration);

    return UNITY_END();
}
