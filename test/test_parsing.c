#include "unity.h"
#include "../src/parsing.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// Tests for decode_announce_list
// ============================================================================

void test_decode_announce_list_valid_single_tracker(void) {
    uint64_t index = 0;
    const char *input = "ll13:http://trackere";
    announce_list_ll *result = decode_announce_list(input, &index, LOG_NO);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(result->list);
    TEST_ASSERT_NULL(result->next);
    
    // Cleanup
    free_announce_list(result);
}

void test_decode_announce_list_valid_multiple_trackers(void) {
    uint64_t index = 0;
    const char *input = "ll13:https://tracker1el13:http://tracker2ee";
    announce_list_ll *result = decode_announce_list(input, &index, LOG_NO);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(result->list);
    TEST_ASSERT_NOT_NULL(result->next);
    TEST_ASSERT_NOT_NULL(result->next->list);
    TEST_ASSERT_NULL(result->next->next);
    
    // Cleanup
    free_announce_list(result);
}

void test_decode_announce_list_valid_multiple_urls_per_tier(void) {
    uint64_t index = 0;
    const char *input = "ll13:https://tracker115:https://tracker1bel13:http://tracker2ee";
    announce_list_ll *result = decode_announce_list(input, &index, LOG_NO);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(result->list);
    
    // Cleanup
    free_announce_list(result);
}

void test_decode_announce_list_valid_empty_list(void) {
    uint64_t index = 0;
    const char *input = "le";
    announce_list_ll *result = decode_announce_list(input, &index, LOG_NO);
    
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_EQUAL_UINT64(2, index);
}

void test_decode_announce_list_valid_nested_empty_lists(void) {
    uint64_t index = 0;
    const char *input = "llelee";
    announce_list_ll *result = decode_announce_list(input, &index, LOG_NO);
    
    TEST_ASSERT_NOT_NULL(result);
    
    // Cleanup
    free_announce_list(result);
}

void test_decode_announce_list_null_index_pointer(void) {
    const char *input = "ll13:http://trackere";
    announce_list_ll *result = decode_announce_list(input, nullptr, LOG_NO);
    
    TEST_ASSERT_NOT_NULL(result);
    
    // Cleanup
    free_announce_list(result);
}

void test_decode_announce_list_invalid_format_no_list(void) {
    uint64_t index = 0;
    const char *input = "13:http://tracker";
    announce_list_ll *result = decode_announce_list(input, &index, LOG_NO);
    
    TEST_ASSERT_NULL(result);
}

void test_decode_announce_list_invalid_format_unclosed(void) {
    uint64_t index = 0;
    const char *input = "ll13:http://tracker";
    announce_list_ll *result = decode_announce_list(input, &index, LOG_NO);
    
    TEST_ASSERT_NULL(result);
}

void test_decode_announce_list_invalid_format_malformed_inner(void) {
    uint64_t index = 0;
    const char *input = "ll99:http://trackere";
    announce_list_ll *result = decode_announce_list(input, &index, LOG_NO);
    
    TEST_ASSERT_NULL(result);
}

void test_decode_announce_list_valid_index_tracking(void) {
    uint64_t index = 0;
    const char *input = "ll13:http://trackeree";
    announce_list_ll *result = decode_announce_list(input, &index, LOG_NO);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_UINT64(21, index);
    
    // Cleanup
    free_announce_list(result);
}

void test_decode_announce_list_valid_udp_tracker(void) {
    uint64_t index = 0;
    const char *input = "ll34:udp://tracker.opentrackr.org:1337ee";
    announce_list_ll *result = decode_announce_list(input, &index, LOG_NO);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(result->list);
    
    // Cleanup
    free_announce_list(result);
}

void test_decode_announce_list_valid_mixed_protocols(void) {
    uint64_t index = 0;
    const char *input = "ll13:https://tracker1el21:udp://tracker2:1337ee";
    announce_list_ll *result = decode_announce_list(input, &index, LOG_NO);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(result->next);
    
    // Cleanup
    free_announce_list(result);
}

void test_decode_announce_list_logging_modes(void) {
    uint64_t index = 0;
    const char *input = "ll13:http://trackere";
    
    announce_list_ll *r1 = decode_announce_list(input, &index, LOG_NO);
    announce_list_ll *r2 = decode_announce_list(input, &index, LOG_ERR);
    announce_list_ll *r3 = decode_announce_list(input, &index, LOG_SUMM);
    announce_list_ll *r4 = decode_announce_list(input, &index, LOG_FULL);
    
    TEST_ASSERT_NOT_NULL(r1);
    TEST_ASSERT_NOT_NULL(r2);
    TEST_ASSERT_NOT_NULL(r3);
    TEST_ASSERT_NOT_NULL(r4);
    
    // Cleanup
    free_announce_list(r1);
    free_announce_list(r2);
    free_announce_list(r3);
    free_announce_list(r4);
}

// ============================================================================
// Tests for read_info_files - Single File Mode
// ============================================================================

void test_read_info_files_single_file_valid_simple(void) {
    uint64_t index = 0;
    const char *input = "d6:lengthi1024e4:name8:file.txtee";
    files_ll *result = read_info_files(input, false, &index, LOG_NO);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(result->file_ptr);
    TEST_ASSERT_NULL(result->next);
    
    // Cleanup
    free_info_files_list(result);
}

void test_read_info_files_single_file_valid_large_file(void) {
    uint64_t index = 0;
    const char *input = "d6:lengthi10737418240e4:name15:largefile.isoee";
    files_ll *result = read_info_files(input, false, &index, LOG_NO);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(result->file_ptr);
    
    // Cleanup
    free_info_files_list(result);
}

void test_read_info_files_single_file_valid_zero_length(void) {
    uint64_t index = 0;
    const char *input = "d6:lengthi0e4:name9:empty.txtee";
    files_ll *result = read_info_files(input, false, &index, LOG_NO);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(result->file_ptr);
    
    // Cleanup
    free_info_files_list(result);
}

void test_read_info_files_single_file_null_index(void) {
    const char *input = "d6:lengthi1024e4:name8:file.txtee";
    files_ll *result = read_info_files(input, false, nullptr, LOG_NO);
    
    TEST_ASSERT_NOT_NULL(result);
    
    // Cleanup
    free_info_files_list(result);
}

void test_read_info_files_single_file_invalid_no_length(void) {
    uint64_t index = 0;
    const char *input = "d4:name8:file.txtee";
    files_ll *result = read_info_files(input, false, &index, LOG_NO);
    
    TEST_ASSERT_NULL(result);
}

void test_read_info_files_single_file_invalid_no_name(void) {
    uint64_t index = 0;
    const char *input = "d6:lengthi1024eee";
    files_ll *result = read_info_files(input, false, &index, LOG_NO);
    
    TEST_ASSERT_NULL(result);
}

void test_read_info_files_single_file_invalid_malformed(void) {
    uint64_t index = 0;
    const char *input = "d6:lengthi1024e4:name";
    files_ll *result = read_info_files(input, false, &index, LOG_NO);
    
    TEST_ASSERT_NULL(result);
}

void test_read_info_files_single_file_index_tracking(void) {
    uint64_t index = 0;
    const char *input = "d6:lengthi1024e4:name8:file.txtee";
    files_ll *result = read_info_files(input, false, &index, LOG_NO);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_GREATER_THAN_UINT64(0, index);
    
    // Cleanup
    free_info_files_list(result);
}

// ============================================================================
// Tests for read_info_files - Multiple Files Mode
// ============================================================================

void test_read_info_files_multiple_files_valid_two_files(void) {
    uint64_t index = 0;
    const char *input = "d5:filesld6:lengthi512e4:pathl5:file1eed6:lengthi1024e4:pathl5:file2eeeee";
    files_ll *result = read_info_files(input, true, &index, LOG_NO);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(result->file_ptr);
    TEST_ASSERT_NOT_NULL(result->next);
    TEST_ASSERT_NOT_NULL(result->next->file_ptr);
    TEST_ASSERT_NULL(result->next->next);
    
    // Cleanup
    free_info_files_list(result);
}

void test_read_info_files_multiple_files_valid_single_file(void) {
    uint64_t index = 0;
    const char *input = "d5:filesld6:lengthi1024e4:pathl8:file.txteeeee";
    files_ll *result = read_info_files(input, true, &index, LOG_NO);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(result->file_ptr);
    TEST_ASSERT_NULL(result->next);
    
    // Cleanup
    free_info_files_list(result);
}

void test_read_info_files_multiple_files_valid_nested_path(void) {
    uint64_t index = 0;
    const char *input = "d5:filesld6:lengthi1024e4:pathl6:folder8:file.txteeeee";
    files_ll *result = read_info_files(input, true, &index, LOG_NO);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(result->file_ptr);
    
    // Cleanup
    free_info_files_list(result);
}

void test_read_info_files_multiple_files_valid_deep_nesting(void) {
    uint64_t index = 0;
    const char *input = "d5:filesld6:lengthi512e4:pathl4:dir14:dir28:file.txteeeee";
    files_ll *result = read_info_files(input, true, &index, LOG_NO);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(result->file_ptr);
    
    // Cleanup
    free_info_files_list(result);
}

void test_read_info_files_multiple_files_valid_empty_files_list(void) {
    uint64_t index = 0;
    const char *input = "d5:fileslee";
    files_ll *result = read_info_files(input, true, &index, LOG_NO);
    
    TEST_ASSERT_NULL(result);
}

void test_read_info_files_multiple_files_valid_many_files(void) {
    uint64_t index = 0;
    const char *input = "d5:filesld6:lengthi100e4:pathl5:file1eed6:lengthi200e4:pathl5:file2eed6:lengthi300e4:pathl5:file3eed6:lengthi400e4:pathl5:file4eed6:lengthi500e4:pathl5:file5eeeee";
    files_ll *result = read_info_files(input, true, &index, LOG_NO);
    
    TEST_ASSERT_NOT_NULL(result);
    
    // Count files
    int count = 0;
    files_ll *current = result;
    while (current != NULL) {
        count++;
        current = current->next;
    }
    TEST_ASSERT_EQUAL_INT(5, count);
    
    // Cleanup
    free_info_files_list(result);
}

void test_read_info_files_multiple_files_null_index(void) {
    const char *input = "d5:filesld6:lengthi1024e4:pathl8:file.txteeeee";
    files_ll *result = read_info_files(input, true, nullptr, LOG_NO);
    
    TEST_ASSERT_NOT_NULL(result);
    
    // Cleanup
    free_info_files_list(result);
}

void test_read_info_files_multiple_files_invalid_no_files_key(void) {
    uint64_t index = 0;
    const char *input = "d4:name8:somename";
    files_ll *result = read_info_files(input, true, &index, LOG_NO);
    
    TEST_ASSERT_NULL(result);
}

void test_read_info_files_multiple_files_invalid_no_length(void) {
    uint64_t index = 0;
    const char *input = "d5:filesld4:pathl8:file.txteeeee";
    files_ll *result = read_info_files(input, true, &index, LOG_NO);
    
    TEST_ASSERT_NULL(result);
}

void test_read_info_files_multiple_files_invalid_no_path(void) {
    uint64_t index = 0;
    const char *input = "d5:filesld6:lengthi1024eeee";
    files_ll *result = read_info_files(input, true, &index, LOG_NO);
    
    TEST_ASSERT_NULL(result);
}

void test_read_info_files_multiple_files_invalid_malformed(void) {
    uint64_t index = 0;
    const char *input = "d5:filesld6:lengthi1024e4:pathl8:file.txt";
    files_ll *result = read_info_files(input, true, &index, LOG_NO);
    
    TEST_ASSERT_NULL(result);
}

void test_read_info_files_multiple_files_index_tracking(void) {
    uint64_t index = 0;
    const char *input = "d5:filesld6:lengthi1024e4:pathl8:file.txteeeee";
    files_ll *result = read_info_files(input, true, &index, LOG_NO);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_GREATER_THAN_UINT64(0, index);
    
    // Cleanup
    free_info_files_list(result);
}

// ============================================================================
// Tests for read_info_files - Edge Cases
// ============================================================================

void test_read_info_files_edge_case_very_long_filename(void) {
    uint64_t index = 0;
    const char *input = "d6:lengthi1024e4:name64:very_long_filename_with_many_characters_to_test_boundary_cases.txtee";
    files_ll *result = read_info_files(input, false, &index, LOG_NO);
    
    TEST_ASSERT_NOT_NULL(result);
    
    // Cleanup
    free_info_files_list(result);
}

void test_read_info_files_edge_case_special_chars_in_name(void) {
    uint64_t index = 0;
    const char *input = "d6:lengthi1024e4:name18:file-name_123.txtee";
    files_ll *result = read_info_files(input, false, &index, LOG_NO);
    
    TEST_ASSERT_NOT_NULL(result);
    
    // Cleanup
    free_info_files_list(result);
}

void test_read_info_files_edge_case_max_uint64_length(void) {
    uint64_t index = 0;
    const char *input = "d6:lengthi18446744073709551615e4:name8:huge.binee";
    files_ll *result = read_info_files(input, false, &index, LOG_NO);
    
    TEST_ASSERT_NOT_NULL(result);
    
    // Cleanup
    free_info_files_list(result);
}

// ============================================================================
// Logging Mode Tests
// ============================================================================

void test_read_info_files_logging_modes_single(void) {
    uint64_t index = 0;
    const char *input = "d6:lengthi1024e4:name8:file.txtee";
    
    files_ll *r1 = read_info_files(input, false, &index, LOG_NO);
    files_ll *r2 = read_info_files(input, false, &index, LOG_ERR);
    files_ll *r3 = read_info_files(input, false, &index, LOG_SUMM);
    files_ll *r4 = read_info_files(input, false, &index, LOG_FULL);
    
    TEST_ASSERT_NOT_NULL(r1);
    TEST_ASSERT_NOT_NULL(r2);
    TEST_ASSERT_NOT_NULL(r3);
    TEST_ASSERT_NOT_NULL(r4);
    
    // Cleanup
    free_info_files_list(r1);
    free_info_files_list(r2);
    free_info_files_list(r3);
    free_info_files_list(r4);
}

void test_read_info_files_logging_modes_multiple(void) {
    uint64_t index = 0;
    const char *input = "d5:filesld6:lengthi1024e4:pathl8:file.txteeeee";
    
    files_ll *r1 = read_info_files(input, true, &index, LOG_NO);
    files_ll *r2 = read_info_files(input, true, &index, LOG_ERR);
    files_ll *r3 = read_info_files(input, true, &index, LOG_SUMM);
    files_ll *r4 = read_info_files(input, true, &index, LOG_FULL);
    
    TEST_ASSERT_NOT_NULL(r1);
    TEST_ASSERT_NOT_NULL(r2);
    TEST_ASSERT_NOT_NULL(r3);
    TEST_ASSERT_NOT_NULL(r4);
    
    // Cleanup
    free_info_files_list(r1);
    free_info_files_list(r2);
    free_info_files_list(r3);
    free_info_files_list(r4);
}

// ============================================================================
// Integration Tests
// ============================================================================

void test_integration_announce_list_complex_structure(void) {
    uint64_t index = 0;
    const char *input = "ll34:udp://tracker1.example.org:1337el34:udp://tracker2.example.org:1337el34:https://tracker3.example.com:8080eee";
    announce_list_ll *result = decode_announce_list(input, &index, LOG_NO);
    
    TEST_ASSERT_NOT_NULL(result);
    
    // Count tiers
    int count = 0;
    announce_list_ll *current = result;
    while (current != NULL) {
        count++;
        current = current->next;
    }
    TEST_ASSERT_EQUAL_INT(3, count);
    
    // Cleanup
    free_announce_list(result);
}

void test_integration_files_list_mixed_sizes(void) {
    uint64_t index = 0;
    const char *input = "d5:filesld6:lengthi0e4:pathl9:empty.txteed6:lengthi1024e4:pathl10:small.dateed6:lengthi1048576e4:pathl9:large.bineed6:lengthi10737418240e4:pathl8:huge.isoeeee";
    files_ll *result = read_info_files(input, true, &index, LOG_NO);
    
    TEST_ASSERT_NOT_NULL(result);
    
    // Count files
    int count = 0;
    files_ll *current = result;
    while (current != NULL) {
        count++;
        current = current->next;
    }
    TEST_ASSERT_EQUAL_INT(4, count);
    
    // Cleanup
    free_info_files_list(result);
}
