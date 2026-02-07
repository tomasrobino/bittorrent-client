//
// Created by robino25 on 28/01/2026.
//

#ifndef BITTORRENT_CLIENT_TEST_FILE_H
#define BITTORRENT_CLIENT_TEST_FILE_H

void test_sha1_to_hex(void);
void test_sha1_to_hex_all_zeros(void);
void test_sha1_to_hex_all_ff(void);
void test_sha1_to_hex_alternating(void);

void test_free_announce_list(void);
void test_free_announce_list_null(void);
void test_free_announce_list_single_node(void);
void test_free_announce_list_multiple_nodes(void);

void test_parse_metainfo(void);
void test_parse_metainfo_empty_string(void);
void test_parse_metainfo_invalid_bencode(void);
void test_parse_metainfo_missing_name(void);
void test_parse_metainfo_private_flag(void);
void test_parse_metainfo_empty_info(void);

void test_free_info_files_list(void);
void test_free_info_files_list_null(void);
void test_free_info_files_list_nested_path(void);
void test_free_info_files_list_multiple_nodes(void);

void test_free_metainfo(void);
void test_free_metainfo_with_files(void);
void test_free_metainfo_with_announce_list(void);

void test_info_t_zero_lengths(void);
void test_info_t_piece_zero(void);


#endif //BITTORRENT_CLIENT_TEST_FILE_H