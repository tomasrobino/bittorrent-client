#include "unity.h"
#include "../src/util.h"

void test_is_digit_normal_digits(void) {
    for (char c = '0'; c <= '9'; c++) {
        TEST_ASSERT_TRUE(is_digit(c));
    }
}

void test_is_digit_outside_digits(void) {
    TEST_ASSERT_FALSE(is_digit('/'));  // just before '0'
    TEST_ASSERT_FALSE(is_digit(':'));  // just after '9'
}

void test_is_digit_letters(void) {
    for (char c = 'a'; c <= 'z'; c++) {
        TEST_ASSERT_FALSE(is_digit(c));
    }
    for (char c = 'A'; c <= 'Z'; c++) {
        TEST_ASSERT_FALSE(is_digit(c));
    }
}

void test_is_digit_non_printable(void) {
    TEST_ASSERT_FALSE(is_digit('\0'));
    TEST_ASSERT_FALSE(is_digit('\n'));
    TEST_ASSERT_FALSE(is_digit('\r'));
    TEST_ASSERT_FALSE(is_digit('\t'));
}

void test_is_digit_high_ascii(void) {
    for (unsigned char c = 128; c < 255; c++) {
        TEST_ASSERT_FALSE(is_digit(c));
    }
}