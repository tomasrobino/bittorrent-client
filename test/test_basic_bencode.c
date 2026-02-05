//
// Created by robino25 on 26/01/2026.
//

#include "unity.h"
#include "../src/basic_bencode.h"
#include <stdlib.h>
#include <string.h>

void test_decode_bencode_string_valid_simple(void) {
    char *result = decode_bencode_string("5:hello", LOG_NO);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("\"hello\"", result);
    free(result);
}

void test_decode_bencode_string_valid_empty(void) {
    char *result = decode_bencode_string("0:", LOG_NO);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("\"\"", result);
    free(result);
}

void test_decode_bencode_string_valid_with_spaces(void) {
    char *result = decode_bencode_string("11:hello world", LOG_NO);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("\"hello world\"", result);
    free(result);
}

void test_decode_bencode_string_valid_with_special_chars(void) {
    char *result = decode_bencode_string("13:hello:world123", LOG_NO);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("\"hello:world123\"", result);
    free(result);
}

void test_decode_bencode_string_valid_single_char(void) {
    char *result = decode_bencode_string("1:a", LOG_NO);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("\"a\"", result);
    free(result);
}

void test_decode_bencode_string_valid_long_string(void) {
    char *result = decode_bencode_string("26:abcdefghijklmnopqrstuvwxyz", LOG_NO);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("\"abcdefghijklmnopqrstuvwxyz\"", result);
    free(result);
}

void test_decode_bencode_string_valid_numbers_in_string(void) {
    char *result = decode_bencode_string("10:1234567890", LOG_NO);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("\"1234567890\"", result);
    free(result);
}

// ============================================================================
// Tests for decode_bencode_int
// ============================================================================

void test_decode_bencode_int_valid_positive(void) {
    char *endptr = nullptr;
    uint64_t result = decode_bencode_int("i42e", &endptr, LOG_NO);
    TEST_ASSERT_EQUAL_UINT64(42, result);
    TEST_ASSERT_NOT_NULL(endptr);
    TEST_ASSERT_EQUAL_CHAR('\0', *endptr);
}

void test_decode_bencode_int_valid_zero(void) {
    char *endptr = nullptr;
    uint64_t result = decode_bencode_int("i0e", &endptr, LOG_NO);
    TEST_ASSERT_EQUAL_UINT64(0, result);
}

void test_decode_bencode_int_valid_large_number(void) {
    char *endptr = nullptr;
    uint64_t result = decode_bencode_int("i1234567890e", &endptr, LOG_NO);
    TEST_ASSERT_EQUAL_UINT64(1234567890, result);
}

void test_decode_bencode_int_valid_with_continuation(void) {
    char *endptr = nullptr;
    uint64_t result = decode_bencode_int("i100eextra", &endptr, LOG_NO);
    TEST_ASSERT_EQUAL_UINT64(100, result);
    TEST_ASSERT_NOT_NULL(endptr);
    TEST_ASSERT_EQUAL_STRING("extra", endptr);
}

void test_decode_bencode_int_null_endptr(void) {
    // Should work even if endptr is nullptr
    uint64_t result = decode_bencode_int("i99e", nullptr, LOG_NO);
    TEST_ASSERT_EQUAL_UINT64(99, result);
}

void test_decode_bencode_int_invalid_format_no_i(void) {
    char *endptr = nullptr;
    uint64_t result = decode_bencode_int("42e", &endptr, LOG_NO);
    TEST_ASSERT_EQUAL_UINT64(0, result);
}

void test_decode_bencode_int_invalid_format_no_e(void) {
    char *endptr = nullptr;
    uint64_t result = decode_bencode_int("i42", &endptr, LOG_NO);
    // TODO
    // Behavior depends on implementation - test what your implementation does
    // This is just an example expectation
}

void test_decode_bencode_int_invalid_not_a_number(void) {
    char *endptr = nullptr;
    uint64_t result = decode_bencode_int("iabce", &endptr, LOG_NO);
    TEST_ASSERT_EQUAL_UINT64(0, result);
}

void test_decode_bencode_int_empty_number(void) {
    char *endptr = nullptr;
    uint64_t result = decode_bencode_int("ie", &endptr, LOG_NO);
    TEST_ASSERT_EQUAL_UINT64(0, result);
}

// ============================================================================
// Tests for decode_bencode_list
// ============================================================================

void test_decode_bencode_list_valid_empty(void) {
    uint32_t length = 0;
    ll *result = decode_bencode_list("le", &length, LOG_NO);
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_EQUAL_UINT32(2, length); // "le" is 2 characters
}

void test_decode_bencode_list_valid_single_string(void) {
    uint32_t length = 0;
    ll *result = decode_bencode_list("l5:helloe", &length, LOG_NO);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(result->val);
    TEST_ASSERT_EQUAL_STRING("\"hello\"", result->val);
    TEST_ASSERT_NULL(result->next);
    free_bencode_list(result);
}

void test_decode_bencode_list_valid_single_int(void) {
    uint32_t length = 0;
    ll *result = decode_bencode_list("li42ee", &length, LOG_NO);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(result->val);
    TEST_ASSERT_EQUAL_STRING("42", result->val);
    TEST_ASSERT_NULL(result->next);
    free_bencode_list(result);
}

void test_decode_bencode_list_valid_multiple_strings(void) {
    uint32_t length = 0;
    ll *result = decode_bencode_list("l5:hello5:worlde", &length, LOG_NO);
    TEST_ASSERT_NOT_NULL(result);

    // First element
    TEST_ASSERT_NOT_NULL(result->val);
    TEST_ASSERT_EQUAL_STRING("\"hello\"", result->val);

    // Second element
    TEST_ASSERT_NOT_NULL(result->next);
    TEST_ASSERT_NOT_NULL(result->next->val);
    TEST_ASSERT_EQUAL_STRING("\"world\"", result->next->val);

    // No third element
    TEST_ASSERT_NULL(result->next->next);

    free_bencode_list(result);
}

void test_decode_bencode_list_valid_mixed_types(void) {
    uint32_t length = 0;
    ll *result = decode_bencode_list("l5:helloi42e3:fooe", &length, LOG_NO);
    TEST_ASSERT_NOT_NULL(result);

    // First element (string)
    TEST_ASSERT_NOT_NULL(result->val);
    TEST_ASSERT_EQUAL_STRING("\"hello\"", result->val);

    // Second element (int)
    TEST_ASSERT_NOT_NULL(result->next);
    TEST_ASSERT_NOT_NULL(result->next->val);
    TEST_ASSERT_EQUAL_STRING("42", result->next->val);

    // Third element (string)
    TEST_ASSERT_NOT_NULL(result->next->next);
    TEST_ASSERT_NOT_NULL(result->next->next->val);
    TEST_ASSERT_EQUAL_STRING("\"foo\"", result->next->next->val);

    // No fourth element
    TEST_ASSERT_NULL(result->next->next->next);

    free_bencode_list(result);
}

void test_decode_bencode_list_valid_nested_list(void) {
    uint32_t length = 0;
    // List containing a string and a nested list
    ll *result = decode_bencode_list("l5:hellol3:fooee", &length, LOG_NO);
    TEST_ASSERT_NOT_NULL(result);

    // First element (string)
    TEST_ASSERT_NOT_NULL(result->val);
    TEST_ASSERT_EQUAL_STRING("\"hello\"", result->val);

    // Second element should be a nested list representation
    TEST_ASSERT_NOT_NULL(result->next);
    TEST_ASSERT_NOT_NULL(result->next->val);

    free_bencode_list(result);
}

void test_decode_bencode_list_invalid_no_l(void) {
    uint32_t length = 0;
    ll *result = decode_bencode_list("5:helloe", &length, LOG_NO);
    TEST_ASSERT_NULL(result);
}

void test_decode_bencode_list_invalid_no_e(void) {
    uint32_t length = 0;
    ll *result = decode_bencode_list("l5:hello", &length, LOG_NO);
    TEST_ASSERT_NULL(result);
}

void test_decode_bencode_list_null_length_pointer(void) {
    // Should work even if length pointer is nullptr
    ll *result = decode_bencode_list("l5:helloe", nullptr, LOG_NO);
    TEST_ASSERT_NOT_NULL(result);
    free_bencode_list(result);
}

void test_decode_bencode_list_valid_length_output(void) {
    uint32_t length = 0;
    ll *result = decode_bencode_list("l5:helloe", &length, LOG_NO);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_UINT32(10, length); // "l5:helloe" is 10 characters
    free_bencode_list(result);
}

// ============================================================================
// Tests for free_bencode_list
// ============================================================================

void test_free_bencode_list_null(void) {
    // Should not crash when freeing nullptr
    free_bencode_list(nullptr);
    TEST_PASS();
}

void test_free_bencode_list_single_element(void) {
    uint32_t length = 0;
    ll *list = decode_bencode_list("l5:helloe", &length, LOG_NO);
    TEST_ASSERT_NOT_NULL(list);

    // Free should not crash
    free_bencode_list(list);
    TEST_PASS();
}

void test_free_bencode_list_multiple_elements(void) {
    uint32_t length = 0;
    ll *list = decode_bencode_list("l5:hello5:worldi42ee", &length, LOG_NO);
    TEST_ASSERT_NOT_NULL(list);

    // Free should not crash
    free_bencode_list(list);
    TEST_PASS();
}

// ============================================================================
// Integration tests
// ============================================================================

void test_integration_parse_complex_list(void) {
    uint32_t length = 0;
    ll *result = decode_bencode_list("l5:helloi123e4:test3:foo0:i0ee", &length, LOG_NO);
    TEST_ASSERT_NOT_NULL(result);

    ll *current = result;
    int count = 0;

    while (current != nullptr) {
        TEST_ASSERT_NOT_NULL(current->val);
        count++;
        current = current->next;
    }

    TEST_ASSERT_EQUAL_INT(6, count); // 6 elements in the list
    free_bencode_list(result);
}

void test_integration_logging_modes(void) {
    // Test that different logging modes don't crash
    char *s1 = decode_bencode_string("5:hello", LOG_NO);
    char *s2 = decode_bencode_string("5:hello", LOG_ERR);
    char *s3 = decode_bencode_string("5:hello", LOG_SUMM);
    char *s4 = decode_bencode_string("5:hello", LOG_FULL);

    TEST_ASSERT_NOT_NULL(s1);
    TEST_ASSERT_NOT_NULL(s2);
    TEST_ASSERT_NOT_NULL(s3);
    TEST_ASSERT_NOT_NULL(s4);

    free(s1);
    free(s2);
    free(s3);
    free(s4);
}