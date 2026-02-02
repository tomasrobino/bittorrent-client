//
// Created by robino25 on 02/02/2026.
//

#ifndef BITTORRENT_CLIENT_TEST_MESSAGES_H
#define BITTORRENT_CLIENT_TEST_MESSAGES_H

// bitfield_to_hex()
void test_bitfield_to_hex_normal(void);
void test_bitfield_to_hex_all_zero(void);
void test_bitfield_to_hex_all_ff(void);
void test_bitfield_to_hex_single_byte(void);
void test_bitfield_to_hex_zero_length(void);

// check_handshake()
void test_check_handshake_valid(void);
void test_check_handshake_wrong_protocol(void);
void test_check_handshake_wrong_info_hash(void);

// process_bitfield()
void test_process_bitfield_identical(void);
void test_process_bitfield_client_empty(void);
void test_process_bitfield_foreign_empty(void);
void test_process_bitfield_zero_size(void);

// read_message_length()
void test_read_message_length_keep_alive(void);
void test_read_message_length_normal(void);

// handle_have()
void test_handle_have_allocates_bitfield(void);
void test_handle_have_peer_already_has_piece(void);
void test_handle_have_client_has_piece(void);
void test_handle_have_client_missing_piece(void);
void test_handle_have_second_byte_piece(void);
void test_handle_have_piece_index_zero(void);
void test_handle_have_last_bit(void);
void test_handle_have_invalid_index(void);

// handle_bitfield()
void test_handle_bitfield_null_payload(void);

// write_block()
void test_write_block_normal(void);
void test_write_block_zero(void);
void test_write_block_null_file(void);

/* TODO
 * Write tests for the following functions. They require mocking
 * try_connect()
 * send_handshake()
 * handle_request()
 * broadcast_have()
 * process_block()
 * handle_piece()
 */


#endif //BITTORRENT_CLIENT_TEST_MESSAGES_H