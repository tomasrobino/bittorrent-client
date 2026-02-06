#ifndef BITTORRENT_CLIENT_TEST_PREDOWNLOAD_UDP_H
#define BITTORRENT_CLIENT_TEST_PREDOWNLOAD_UDP_H

//
// Created by Claude on 2/5/2026.
// Test function declarations for test_connection.c
//

#ifndef TEST_CONNECTION_H
#define TEST_CONNECTION_H

// Setup and teardown
void setUp(void);
void tearDown(void);

// split_address tests
void test_split_address_valid_udp_ipv4(void);
void test_split_address_valid_http_ipv4(void);
void test_split_address_valid_https_ipv4(void);
void test_split_address_valid_udp_default_port(void);
void test_split_address_valid_ipv4_address(void);
void test_split_address_valid_ipv6_address(void);
void test_split_address_valid_hostname_with_subdomain(void);
void test_split_address_valid_high_port_number(void);
void test_split_address_invalid_no_protocol(void);
void test_split_address_invalid_no_port(void);
void test_split_address_invalid_malformed_url(void);
void test_split_address_invalid_empty_string(void);
void test_split_address_invalid_null_pointer(void);

// shuffle_address_array tests
void test_shuffle_address_array_single_element(void);
void test_shuffle_address_array_two_elements(void);
void test_shuffle_address_array_multiple_elements(void);
void test_shuffle_address_array_zero_length(void);
void test_shuffle_address_array_null_array(void);

// url_to_ip tests
void test_url_to_ip_valid_ipv4_address(void);
void test_url_to_ip_valid_ipv6_address(void);
void test_url_to_ip_valid_hostname_resolution(void);
void test_url_to_ip_invalid_hostname(void);
void test_url_to_ip_null_address(void);
void test_url_to_ip_logging_modes(void);

// try_request_udp tests
void test_try_request_udp_single_tracker_success(void);
void test_try_request_udp_multiple_trackers(void);
void test_try_request_udp_all_fail(void);
void test_try_request_udp_null_sockfd(void);
void test_try_request_udp_null_request(void);
void test_try_request_udp_zero_amount(void);

// connect_request_udp tests
void test_connect_request_udp_valid_single_tracker(void);
void test_connect_request_udp_valid_multiple_trackers(void);
void test_connect_request_udp_null_successful_index(void);
void test_connect_request_udp_null_sockfd(void);
void test_connect_request_udp_zero_amount(void);

// connect_udp tests
void test_connect_udp_valid_single_tracker(void);
void test_connect_udp_valid_multiple_trackers(void);
void test_connect_udp_null_announce_list(void);
void test_connect_udp_null_connection_data(void);
void test_connect_udp_zero_amount(void);

// announce_request_udp tests
void test_announce_request_udp_valid_request(void);
void test_announce_request_udp_null_server_addr(void);
void test_announce_request_udp_null_info_hash(void);
void test_announce_request_udp_null_peer_id(void);
void test_announce_request_udp_null_torrent_stats(void);
void test_announce_request_udp_invalid_sockfd(void);
void test_announce_request_udp_zero_connection_id(void);

// scrape_request_udp tests
void test_scrape_request_udp_valid_single_torrent(void);
void test_scrape_request_udp_valid_multiple_torrents(void);
void test_scrape_request_udp_null_server_addr(void);
void test_scrape_request_udp_null_info_hash(void);
void test_scrape_request_udp_invalid_sockfd(void);
void test_scrape_request_udp_zero_torrent_amount(void);

#endif // TEST_CONNECTION_H

#endif //BITTORRENT_CLIENT_TEST_PREDOWNLOAD_UDP_H