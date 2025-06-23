extern "C" {
    #include "../src/structs.h"
    #include "../src/connection.h"
    #include <stdlib.h>
}

#include <catch2/catch_test_macros.hpp>

// For some reason Catch2 just segfaults here, but the function works correctly
TEST_CASE("parse_address detects protocol correctly") {
    SECTION("UDP address") {
        address_t* addr = split_address("udp://example.com:1234");
        REQUIRE(addr->protocol == UDP);
        REQUIRE(std::string(addr->host) == "example.com");
        REQUIRE(addr->port == "1234");
        free(addr->host);
        free(addr);
    }

    SECTION("HTTPS address") {
        address_t* addr = split_address("https://secure.com:443");
        REQUIRE(addr->protocol == HTTPS);
        REQUIRE(std::string(addr->host) == "secure.com");
        REQUIRE(addr->port == "443");
        free(addr->host);
        free(addr);
    }

    SECTION("Default HTTP address") {
        address_t* addr = split_address("http://plain.com:80");
        REQUIRE(addr->protocol == HTTP);
        REQUIRE(std::string(addr->host) == "plain.com");
        REQUIRE(addr->port == "80");
        free(addr->host);
        free(addr);
    }
}


TEST_CASE("url_to_ip resolves known hostname", "[network]") {
    SECTION("No path") {
        const address_t* address = split_address("udp://tracker.leechers-paradise.org:6969");
        char* ip = url_to_ip(*address);

        REQUIRE(ip != nullptr);

        REQUIRE(
            strcmp(ip, "199.59.243.228") == 0
        );
        free(ip);
    }

    SECTION("With path") {
        const address_t* address = split_address("udp://tracker.opentrackr.org:1337/announce");
        char* ip = url_to_ip(*address);

        REQUIRE(ip != nullptr);

        REQUIRE(
            strcmp(ip, "93.158.213.92") == 0
        );
        free(ip);
    }

    SECTION("UDP IPv6") {
        const address_t* address = split_address("udp://tracker.torrent.eu.org:451/announce");
        char* ip = url_to_ip(*address);

        REQUIRE(ip != nullptr);

        REQUIRE(
            strcmp(ip, "2a03:7220:8083:cd00::1") == 0
        );
        free(ip);
    }

    SECTION("HTTP IPv4") {
        const address_t* address = split_address("http://tracker.mywaifu.best:6969/announce");
        char* ip = url_to_ip(*address);

        REQUIRE(ip != nullptr);

        REQUIRE(
            strcmp(ip, "95.217.167.10") == 0
        );
        free(ip);
    }

    SECTION("HTTP IPv6") {
        const address_t* address = split_address("http://tracker.bt4g.com:2095/announce");
        char* ip = url_to_ip(*address);

        REQUIRE(ip != nullptr);

        if (strcmp(ip, "2606:4700:3034::6815:39b6") == 0) {
            REQUIRE(
                strcmp(ip, "2606:4700:3034::6815:39b6") == 0
            );
        } else {
            REQUIRE(
                strcmp(ip, "2606:4700:3037::ac43:a548") == 0
            );
        }
        free(ip);
    }

    SECTION("HTTPS IPv4") {
        const address_t* address = split_address("https://tr-zhuqiy-1.dgj055.icu:443/announce");
        char* ip = url_to_ip(*address);

        REQUIRE(ip != nullptr);

        if (strcmp(ip, "95.216.3.28") == 0) {
            REQUIRE(
                strcmp(ip, "95.216.3.28") == 0
            );
        } else {
            REQUIRE(
                strcmp(ip, "69.197.155.114") == 0
            );
        }
        free(ip);
    }

    SECTION("HTTPS IPv6") {
        const address_t* address = split_address("https://tracker.expli.top:443/announce");
        char* ip = url_to_ip(*address);

        REQUIRE(ip != nullptr);

        REQUIRE(
            strcmp(ip, "2001:41d0:700:1f37::1") == 0
        );
        free(ip);
    }
}