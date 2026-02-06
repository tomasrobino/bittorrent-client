#include "unity.h"
#include "../src/predownload_udp.h"
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

// ============================================================================
// Tests for split_address
// ============================================================================

void test_split_address_valid_udp_ipv4(void) {
    address_t *result = split_address("udp://tracker.example.com:6969");
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(UDP, result->protocol);
    TEST_ASSERT_EQUAL_STRING("tracker.example.com", result->host);
    TEST_ASSERT_EQUAL_STRING("6969", result->port);
    
    // Cleanup
    // free_address(result);
}

void test_split_address_valid_http_ipv4(void) {
    address_t *result = split_address("http://tracker.example.com:8080");
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(HTTP, result->protocol);
    TEST_ASSERT_EQUAL_STRING("tracker.example.com", result->host);
    TEST_ASSERT_EQUAL_STRING("8080", result->port);
    
    // Cleanup
    // free_address(result);
}

void test_split_address_valid_https_ipv4(void) {
    address_t *result = split_address("https://tracker.example.com:443");
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(HTTPS, result->protocol);
    TEST_ASSERT_EQUAL_STRING("tracker.example.com", result->host);
    TEST_ASSERT_EQUAL_STRING("443", result->port);
    
    // Cleanup
    // free_address(result);
}

void test_split_address_valid_udp_default_port(void) {
    address_t *result = split_address("udp://tracker.example.com:80");
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("80", result->port);
    
    // Cleanup
    // free_address(result);
}

void test_split_address_valid_ipv4_address(void) {
    address_t *result = split_address("udp://192.168.1.1:6969");
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("192.168.1.1", result->host);
    TEST_ASSERT_EQUAL_STRING("6969", result->port);
    
    // Cleanup
    // free_address(result);
}

void test_split_address_valid_ipv6_address(void) {
    address_t *result = split_address("udp://[2001:db8::1]:6969");
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("2001:db8::1", result->host);
    TEST_ASSERT_EQUAL_STRING("6969", result->port);
    
    // Cleanup
    // free_address(result);
}

void test_split_address_valid_hostname_with_subdomain(void) {
    address_t *result = split_address("udp://tracker.sub.example.com:6969");
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("tracker.sub.example.com", result->host);
    
    // Cleanup
    // free_address(result);
}

void test_split_address_valid_high_port_number(void) {
    address_t *result = split_address("udp://tracker.example.com:65535");
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("65535", result->port);
    
    // Cleanup
    // free_address(result);
}

void test_split_address_invalid_no_protocol(void) {
    address_t *result = split_address("tracker.example.com:6969");
    
    TEST_ASSERT_NULL(result);
}

void test_split_address_invalid_no_port(void) {
    address_t *result = split_address("udp://tracker.example.com");
    
    TEST_ASSERT_NULL(result);
}

void test_split_address_invalid_malformed_url(void) {
    address_t *result = split_address("udp:/tracker.example.com:6969");
    
    TEST_ASSERT_NULL(result);
}

void test_split_address_invalid_empty_string(void) {
    address_t *result = split_address("");
    
    TEST_ASSERT_NULL(result);
}

void test_split_address_invalid_null_pointer(void) {
    address_t *result = split_address(nullptr);
    
    TEST_ASSERT_NULL(result);
}

// ============================================================================
// Tests for shuffle_address_array
// ============================================================================

void test_shuffle_address_array_single_element(void) {
    address_t addr1 = {.host = "tracker1.com", .port = "6969", .protocol = UDP};
    address_t *array[] = {&addr1};
    
    // Should not crash
    shuffle_address_array(array, 1);
    
    // Single element should remain unchanged
    TEST_ASSERT_EQUAL_PTR(&addr1, array[0]);
}

void test_shuffle_address_array_two_elements(void) {
    address_t addr1 = {.host = "tracker1.com", .port = "6969", .protocol = UDP};
    address_t addr2 = {.host = "tracker2.com", .port = "6969", .protocol = UDP};
    address_t *array[] = {&addr1, &addr2};
    
    shuffle_address_array(array, 2);
    
    // Both elements should still be in array
    int found1 = 0, found2 = 0;
    for (int i = 0; i < 2; i++) {
        if (array[i] == &addr1) found1 = 1;
        if (array[i] == &addr2) found2 = 1;
    }
    TEST_ASSERT_EQUAL_INT(1, found1);
    TEST_ASSERT_EQUAL_INT(1, found2);
}

void test_shuffle_address_array_multiple_elements(void) {
    address_t addr1 = {.host = "tracker1.com", .port = "6969", .protocol = UDP};
    address_t addr2 = {.host = "tracker2.com", .port = "6969", .protocol = UDP};
    address_t addr3 = {.host = "tracker3.com", .port = "6969", .protocol = UDP};
    address_t addr4 = {.host = "tracker4.com", .port = "6969", .protocol = UDP};
    address_t addr5 = {.host = "tracker5.com", .port = "6969", .protocol = UDP};
    address_t *array[] = {&addr1, &addr2, &addr3, &addr4, &addr5};
    
    shuffle_address_array(array, 5);
    
    // All elements should still be present
    int found[5] = {0, 0, 0, 0, 0};
    for (int i = 0; i < 5; i++) {
        if (array[i] == &addr1) found[0] = 1;
        if (array[i] == &addr2) found[1] = 1;
        if (array[i] == &addr3) found[2] = 1;
        if (array[i] == &addr4) found[3] = 1;
        if (array[i] == &addr5) found[4] = 1;
    }
    for (int i = 0; i < 5; i++) {
        TEST_ASSERT_EQUAL_INT(1, found[i]);
    }
}

void test_shuffle_address_array_zero_length(void) {
    address_t *array[] = {nullptr};
    
    // Should not crash
    shuffle_address_array(array, 0);
    TEST_PASS();
}

void test_shuffle_address_array_null_array(void) {
    // Should not crash
    shuffle_address_array(nullptr, 5);
    TEST_PASS();
}

// ============================================================================
// Tests for url_to_ip
// ============================================================================

void test_url_to_ip_valid_ipv4_address(void) {
    address_t addr = {
        .host = "192.168.1.1",
        .port = "6969",
        .protocol = UDP,
        .ip_version = 4
    };
    
    char *result = url_to_ip(&addr, LOG_NO);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("192.168.1.1", result);
    
    free(result);
}

void test_url_to_ip_valid_ipv6_address(void) {
    address_t addr = {
        .host = "2001:db8::1",
        .port = "6969",
        .protocol = UDP,
        .ip_version = 6
    };
    
    char *result = url_to_ip(&addr, LOG_NO);
    
    TEST_ASSERT_NOT_NULL(result);
    // IPv6 address should be returned
    TEST_ASSERT_NOT_NULL(strstr(result, ":"));
    
    free(result);
}

void test_url_to_ip_valid_hostname_resolution(void) {
    address_t addr = {
        .host = "localhost",
        .port = "6969",
        .protocol = UDP,
        .ip_version = 4
    };
    
    char *result = url_to_ip(&addr, LOG_NO);
    
    TEST_ASSERT_NOT_NULL(result);
    // localhost should resolve to 127.0.0.1
    TEST_ASSERT_EQUAL_STRING("127.0.0.1", result);
    
    free(result);
}

void test_url_to_ip_invalid_hostname(void) {
    address_t addr = {
        .host = "this.domain.definitely.does.not.exist.invalid",
        .port = "6969",
        .protocol = UDP,
        .ip_version = 4
    };
    
    char *result = url_to_ip(&addr, LOG_NO);
    
    TEST_ASSERT_NULL(result);
}

void test_url_to_ip_null_address(void) {
    char *result = url_to_ip(nullptr, LOG_NO);
    
    TEST_ASSERT_NULL(result);
}

void test_url_to_ip_logging_modes(void) {
    address_t addr = {
        .host = "127.0.0.1",
        .port = "6969",
        .protocol = UDP,
        .ip_version = 4
    };
    
    char *r1 = url_to_ip(&addr, LOG_NO);
    char *r2 = url_to_ip(&addr, LOG_ERR);
    char *r3 = url_to_ip(&addr, LOG_SUMM);
    char *r4 = url_to_ip(&addr, LOG_FULL);
    
    TEST_ASSERT_NOT_NULL(r1);
    TEST_ASSERT_NOT_NULL(r2);
    TEST_ASSERT_NOT_NULL(r3);
    TEST_ASSERT_NOT_NULL(r4);
    
    free(r1);
    free(r2);
    free(r3);
    free(r4);
}

// ============================================================================
// Tests for try_request_udp
// ============================================================================

void test_try_request_udp_single_tracker_success(void) {
    // Mock setup
    int32_t sockfd[1] = {3};
    connect_request_t req = {0x41727101980, 0, 12345};
    const void *req_ptr[1] = {&req};
    struct sockaddr_in server = {0};
    const struct sockaddr *server_addr[1] = {(struct sockaddr *)&server};
    
    // This test would require mocking socket operations
    // Placeholder for actual implementation testing
    TEST_IGNORE_MESSAGE("Requires socket mocking");
}

void test_try_request_udp_multiple_trackers(void) {
    // This test would require mocking socket operations
    TEST_IGNORE_MESSAGE("Requires socket mocking");
}

void test_try_request_udp_all_fail(void) {
    // This test would require mocking socket operations
    TEST_IGNORE_MESSAGE("Requires socket mocking");
}

void test_try_request_udp_null_sockfd(void) {
    connect_request_t req = {0x41727101980, 0, 12345};
    const void *req_ptr[1] = {&req};
    struct sockaddr_in server = {0};
    const struct sockaddr *server_addr[1] = {(struct sockaddr *)&server};
    
    int32_t *result = try_request_udp(1, nullptr, req_ptr, sizeof(req), server_addr, LOG_NO);
    
    TEST_ASSERT_NULL(result);
}

void test_try_request_udp_null_request(void) {
    int32_t sockfd[1] = {3};
    struct sockaddr_in server = {0};
    const struct sockaddr *server_addr[1] = {(struct sockaddr *)&server};
    
    int32_t *result = try_request_udp(1, sockfd, nullptr, sizeof(connect_request_t), server_addr, LOG_NO);
    
    TEST_ASSERT_NULL(result);
}

void test_try_request_udp_zero_amount(void) {
    int32_t sockfd[1] = {3};
    connect_request_t req = {0x41727101980, 0, 12345};
    const void *req_ptr[1] = {&req};
    struct sockaddr_in server = {0};
    const struct sockaddr *server_addr[1] = {(struct sockaddr *)&server};
    
    int32_t *result = try_request_udp(0, sockfd, req_ptr, sizeof(req), server_addr, LOG_NO);
    
    TEST_ASSERT_NULL(result);
}

// ============================================================================
// Tests for connect_request_udp
// ============================================================================

void test_connect_request_udp_valid_single_tracker(void) {
    // This test would require mocking socket operations
    TEST_IGNORE_MESSAGE("Requires socket mocking");
}

void test_connect_request_udp_valid_multiple_trackers(void) {
    // This test would require mocking socket operations
    TEST_IGNORE_MESSAGE("Requires socket mocking");
}

void test_connect_request_udp_null_successful_index(void) {
    int32_t sockfd[1] = {3};
    struct sockaddr_in server = {0};
    const struct sockaddr *server_addr[1] = {(struct sockaddr *)&server};
    
    // Should work even with nullptr successful_index
    // This test would require mocking socket operations
    TEST_IGNORE_MESSAGE("Requires socket mocking");
}

void test_connect_request_udp_null_sockfd(void) {
    struct sockaddr_in server = {0};
    const struct sockaddr *server_addr[1] = {(struct sockaddr *)&server};
    int32_t index = 0;
    
    uint64_t result = connect_request_udp(server_addr, nullptr, 1, &index, LOG_NO);
    
    TEST_ASSERT_EQUAL_UINT64(0, result);
}

void test_connect_request_udp_zero_amount(void) {
    int32_t sockfd[1] = {3};
    struct sockaddr_in server = {0};
    const struct sockaddr *server_addr[1] = {(struct sockaddr *)&server};
    int32_t index = 0;
    
    uint64_t result = connect_request_udp(server_addr, sockfd, 0, &index, LOG_NO);
    
    TEST_ASSERT_EQUAL_UINT64(0, result);
}

// ============================================================================
// Tests for connect_udp
// ============================================================================

void test_connect_udp_valid_single_tracker(void) {
    // This test would require full mock setup
    TEST_IGNORE_MESSAGE("Requires full tracker mock");
}

void test_connect_udp_valid_multiple_trackers(void) {
    // This test would require full mock setup
    TEST_IGNORE_MESSAGE("Requires full tracker mock");
}

void test_connect_udp_null_announce_list(void) {
    connection_data_t conn_data = {0};
    int32_t index = 0;
    
    uint64_t result = connect_udp(1, nullptr, &index, &conn_data, LOG_NO);
    
    TEST_ASSERT_EQUAL_UINT64(0, result);
}

void test_connect_udp_null_connection_data(void) {
    // Create a proper announce_list_ll with nested ll structure
    ll tracker = {.next = nullptr, .val = "udp://tracker.example.com:6969"};
    announce_list_ll list = {.next = nullptr, .list = &tracker};
    int32_t index = 0;
    
    uint64_t result = connect_udp(1, &list, &index, nullptr, LOG_NO);
    
    TEST_ASSERT_EQUAL_UINT64(0, result);
}

void test_connect_udp_zero_amount(void) {
    // Create a proper announce_list_ll with nested ll structure
    ll tracker = {.next = nullptr, .val = "udp://tracker.example.com:6969"};
    announce_list_ll list = {.next = nullptr, .list = &tracker};
    connection_data_t conn_data = {0};
    int32_t index = 0;
    
    uint64_t result = connect_udp(0, &list, &index, &conn_data, LOG_NO);
    
    TEST_ASSERT_EQUAL_UINT64(0, result);
}

// ============================================================================
// Tests for announce_request_udp
// ============================================================================

void test_announce_request_udp_valid_request(void) {
    // This test would require mocking socket operations
    TEST_IGNORE_MESSAGE("Requires socket mocking");
}

void test_announce_request_udp_null_server_addr(void) {
    unsigned char info_hash[20] = {0};
    unsigned char peer_id[20] = {0};
    torrent_stats_t stats = {0, 1024, 0, 0, 12345};
    
    announce_response_t *result = announce_request_udp(
        nullptr, 3, 123456789, info_hash, peer_id, &stats, 6881, LOG_NO
    );
    
    TEST_ASSERT_NULL(result);
}

void test_announce_request_udp_null_info_hash(void) {
    struct sockaddr_in server = {0};
    unsigned char peer_id[20] = {0};
    torrent_stats_t stats = {0, 1024, 0, 0, 12345};
    
    announce_response_t *result = announce_request_udp(
        (struct sockaddr *)&server, 3, 123456789, nullptr, peer_id, &stats, 6881, LOG_NO
    );
    
    TEST_ASSERT_NULL(result);
}

void test_announce_request_udp_null_peer_id(void) {
    struct sockaddr_in server = {0};
    unsigned char info_hash[20] = {0};
    torrent_stats_t stats = {0, 1024, 0, 0, 12345};
    
    announce_response_t *result = announce_request_udp(
        (struct sockaddr *)&server, 3, 123456789, info_hash, nullptr, &stats, 6881, LOG_NO
    );
    
    TEST_ASSERT_NULL(result);
}

void test_announce_request_udp_null_torrent_stats(void) {
    struct sockaddr_in server = {0};
    unsigned char info_hash[20] = {0};
    unsigned char peer_id[20] = {0};
    
    announce_response_t *result = announce_request_udp(
        (struct sockaddr *)&server, 3, 123456789, info_hash, peer_id, nullptr, 6881, LOG_NO
    );
    
    TEST_ASSERT_NULL(result);
}

void test_announce_request_udp_invalid_sockfd(void) {
    struct sockaddr_in server = {0};
    unsigned char info_hash[20] = {0};
    unsigned char peer_id[20] = {0};
    torrent_stats_t stats = {0, 1024, 0, 0, 12345};
    
    announce_response_t *result = announce_request_udp(
        (struct sockaddr *)&server, -1, 123456789, info_hash, peer_id, &stats, 6881, LOG_NO
    );
    
    TEST_ASSERT_NULL(result);
}

void test_announce_request_udp_zero_connection_id(void) {
    struct sockaddr_in server = {0};
    unsigned char info_hash[20] = {0};
    unsigned char peer_id[20] = {0};
    torrent_stats_t stats = {0, 1024, 0, 0, 12345};
    
    // Zero connection_id might be invalid depending on implementation
    // This test documents the expected behavior
    TEST_IGNORE_MESSAGE("Behavior depends on implementation");
}

// ============================================================================
// Tests for scrape_request_udp
// ============================================================================

void test_scrape_request_udp_valid_single_torrent(void) {
    // This test would require mocking socket operations
    TEST_IGNORE_MESSAGE("Requires socket mocking");
}

void test_scrape_request_udp_valid_multiple_torrents(void) {
    // This test would require mocking socket operations
    TEST_IGNORE_MESSAGE("Requires socket mocking");
}

void test_scrape_request_udp_null_server_addr(void) {
    char info_hash[20] = {0};
    
    scrape_response_t *result = scrape_request_udp(
        nullptr, 3, 123456789, info_hash, 1, LOG_NO
    );
    
    TEST_ASSERT_NULL(result);
}

void test_scrape_request_udp_null_info_hash(void) {
    struct sockaddr_in server = {0};
    
    scrape_response_t *result = scrape_request_udp(
        (struct sockaddr *)&server, 3, 123456789, nullptr, 1, LOG_NO
    );
    
    TEST_ASSERT_NULL(result);
}

void test_scrape_request_udp_invalid_sockfd(void) {
    struct sockaddr_in server = {0};
    char info_hash[20] = {0};
    
    scrape_response_t *result = scrape_request_udp(
        (struct sockaddr *)&server, -1, 123456789, info_hash, 1, LOG_NO
    );
    
    TEST_ASSERT_NULL(result);
}

void test_scrape_request_udp_zero_torrent_amount(void) {
    struct sockaddr_in server = {0};
    char info_hash[20] = {0};
    
    scrape_response_t *result = scrape_request_udp(
        (struct sockaddr *)&server, 3, 123456789, info_hash, 0, LOG_NO
    );
    
    TEST_ASSERT_NULL(result);
}
