//
// Created by Tomas on 5/2/2026.
//

#ifndef BITTORRENT_CLIENT_TEST_BASIC_BENCODE_H
#define BITTORRENT_CLIENT_TEST_BASIC_BENCODE_H

// String decoding tests
void test_decode_bencode_string_valid_empty(void);
void test_decode_bencode_string_valid_long_string(void);
void test_decode_bencode_string_valid_numbers_in_string(void);
void test_decode_bencode_string_valid_simple(void);
void test_decode_bencode_string_valid_single_char(void);
void test_decode_bencode_string_valid_with_spaces(void);
void test_decode_bencode_string_valid_with_special_chars(void);

// Integer decoding tests
void test_decode_bencode_int_empty_number(void);
void test_decode_bencode_int_invalid_format_no_e(void);
void test_decode_bencode_int_invalid_format_no_i(void);
void test_decode_bencode_int_invalid_not_a_number(void);
void test_decode_bencode_int_null_endptr(void);
void test_decode_bencode_int_valid_large_number(void);
void test_decode_bencode_int_valid_positive(void);
void test_decode_bencode_int_valid_with_continuation(void);
void test_decode_bencode_int_valid_zero(void);

// List decoding tests
void test_decode_bencode_list_invalid_no_e(void);
void test_decode_bencode_list_invalid_no_l(void);
void test_decode_bencode_list_null_length_pointer(void);
void test_decode_bencode_list_valid_empty(void);
void test_decode_bencode_list_valid_length_output(void);
void test_decode_bencode_list_valid_mixed_types(void);
void test_decode_bencode_list_valid_multiple_strings(void);
void test_decode_bencode_list_valid_nested_list(void);
void test_decode_bencode_list_valid_single_int(void);
void test_decode_bencode_list_valid_single_string(void);

// Free list tests
void test_free_bencode_list_multiple_elements(void);
void test_free_bencode_list_null(void);
void test_free_bencode_list_single_element(void);

// Integration tests
void test_integration_logging_modes(void);
void test_integration_parse_complex_list(void);


#endif //BITTORRENT_CLIENT_TEST_BASIC_BENCODE_H