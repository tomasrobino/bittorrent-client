extern "C" {
    #include "../src/structs.h"
    #include "../src/connection.h"
    #include <stdlib.h>
}

#include <catch2/catch_test_macros.hpp>

TEST_CASE("parse_address detects protocol correctly") {
    SECTION("UDP address") {
        address_t* addr = parse_address("udp://example.com:1234");
        REQUIRE(addr->protocol == UDP);
        REQUIRE(std::string(addr->host) == "example.com:1234");
        REQUIRE(addr->port == 1234);
        free(addr);
    }

    SECTION("HTTPS address") {
        address_t* addr = parse_address("https://secure.com:443");
        REQUIRE(addr->protocol == HTTPS);
        REQUIRE(std::string(addr->host) == "secure.com:443");
        REQUIRE(addr->port == 443);
        free(addr);
    }

    SECTION("Default HTTP address") {
        address_t* addr = parse_address("http://plain.com:80");
        REQUIRE(addr->protocol == HTTP);
        REQUIRE(std::string(addr->host) == "plain.com:80");
        REQUIRE(addr->port == 80);
        free(addr);
    }
}
