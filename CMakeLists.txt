cmake_minimum_required(VERSION 3.13)

project(bittorrent-client)
enable_testing()
include(FetchContent)
set(CMAKE_C_STANDARD 23) # Enable the C23 standard
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)

# creating bittorent_core library
add_library(bittorrent_core ${SOURCE_FILES}
        src/util.h
        src/parsing.c
        src/basic_bencode.c
        src/parsing.h
        src/magnet.c
        src/file.c
        src/predownload_udp.c
        src/predownload_udp.h
        src/downloading.c
        src/downloading.h
        src/messages.c
        src/messages.h
        src/file.h
        src/magnet.h
        src/util.c
        src/downloading_types.h
        src/messages_types.h
        src/basic_bencode.h
)

# Link OpenSSL, CURL and Math library
target_link_libraries(bittorrent_core PRIVATE OpenSSL::Crypto CURL::libcurl m )

# building main executable
add_executable(bittorrent src/main.c)

# linking bittorrent with bittorrent_core
target_link_libraries(bittorrent PRIVATE bittorrent_core)

# fetch Unity
FetchContent_Declare(
        unity
        GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
        GIT_TAG v2.5.2
)
FetchContent_MakeAvailable(unity)

# creating bittorrent_tests executable
add_executable(bittorrent_tests
        test/test_parsing.c
        test/test_basic_bencode.c
        test/test_magnet.c
        test/test_file.c
        test/test_predownload_udp.c
        test/test_downloading.c
        test/test_messages.c
        test/test_util.c
        test/test_runner.c
        test/test_util.h
        test/test_file.h
        test/test_messages.h
        test/test_basic_bencode.h
)

# linking bittorrent_tests with bittorrent_core
target_link_libraries(bittorrent_tests PRIVATE unity bittorrent_core)

# making tests recognizable to ctest
add_test(
        NAME bittorrent_tests
        COMMAND bittorrent_tests
)

file(GLOB TORRENT_FILES "${CMAKE_SOURCE_DIR}/test-files/*.torrent")

foreach(torrent ${TORRENT_FILES})
    get_filename_component(fname ${torrent} NAME)
    configure_file(${torrent} ${CMAKE_BINARY_DIR}/test-files/${fname} COPYONLY)
endforeach()