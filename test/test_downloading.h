#ifndef BITTORRENT_CLIENT_TEST_DOWNLOADING_H
#define BITTORRENT_CLIENT_TEST_DOWNLOADING_H

// calc_block_size tests
void test_calc_block_size_regular_block(void);
void test_calc_block_size_second_block(void);
void test_calc_block_size_last_block_full_size(void);
void test_calc_block_size_last_block_partial(void);
void test_calc_block_size_small_piece(void);
void test_calc_block_size_exact_block_size_piece(void);
void test_calc_block_size_zero_offset(void);
void test_calc_block_size_offset_near_end(void);

// get_path tests
void test_get_path_single_segment(void);
void test_get_path_two_segments(void);
void test_get_path_multiple_segments(void);
void test_get_path_deep_nesting(void);
void test_get_path_null_filepath(void);
void test_get_path_logging_modes(void);

// piece_complete tests
void test_piece_complete_single_block_piece_complete(void);
void test_piece_complete_single_block_piece_incomplete(void);
void test_piece_complete_multi_block_piece_complete(void);
void test_piece_complete_multi_block_piece_partial(void);
void test_piece_complete_last_piece_smaller(void);
void test_piece_complete_null_block_tracker(void);

// are_bits_set tests
void test_are_bits_set_all_set_single_byte(void);
void test_are_bits_set_none_set(void);
void test_are_bits_set_partial(void);
void test_are_bits_set_partial_fail(void);
void test_are_bits_set_single_bit_set(void);
void test_are_bits_set_single_bit_not_set(void);
void test_are_bits_set_cross_byte_boundary(void);
void test_are_bits_set_cross_byte_boundary_partial(void);
void test_are_bits_set_middle_range(void);
void test_are_bits_set_null_bitfield(void);
void test_are_bits_set_start_greater_than_end(void);

// closing_files tests
void test_closing_files_null_files(void);
void test_closing_files_null_bitfield(void);
void test_closing_files_single_file_complete(void);
void test_closing_files_single_file_incomplete(void);
void test_closing_files_multiple_files(void);

// handle_predownload_udp tests
void test_handle_predownload_udp_valid_request(void);
void test_handle_predownload_udp_null_peer_id(void);
void test_handle_predownload_udp_null_torrent_stats(void);
void test_handle_predownload_udp_logging_modes(void);

// read_from_socket tests
void test_read_from_socket_null_peer(void);
void test_read_from_socket_invalid_epoll(void);
void test_read_from_socket_valid_operation(void);
void test_read_from_socket_connection_closed(void);
void test_read_from_socket_partial_read(void);

// reconnect tests
void test_reconnect_null_peer_list(void);
void test_reconnect_zero_peer_amount(void);
void test_reconnect_invalid_epoll(void);
void test_reconnect_no_closed_peers(void);
void test_reconnect_some_closed_peers(void);

// write_state tests
void test_write_state_null_filename(void);
void test_write_state_null_state(void);
void test_write_state_valid_state(void);
void test_write_state_large_state(void);

// read_state tests
void test_read_state_null_filename(void);
void test_read_state_nonexistent_file(void);
void test_read_state_valid_file(void);
void test_read_state_corrupted_file(void);

// init_state tests
void test_init_state_new_file(void);
void test_init_state_existing_file(void);
void test_init_state_null_filename(void);
void test_init_state_null_bitfield(void);
void test_init_state_zero_piece_count(void);

// torrent tests
void test_torrent_null_peer_id(void);
void test_torrent_invalid_metainfo(void);
void test_torrent_full_integration(void);

#endif //BITTORRENT_CLIENT_TEST_DOWNLOADING_H