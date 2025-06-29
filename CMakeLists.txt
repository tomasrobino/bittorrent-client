cmake_minimum_required(VERSION 3.13)

project(bittorrent-client)
enable_testing()
add_subdirectory(test)

file(GLOB_RECURSE SOURCE_FILES src/*.c src/*.h)

set(CMAKE_C_STANDARD 23) # Enable the C23 standard

find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)

add_executable(bittorrent ${SOURCE_FILES}
        src/structs.h
        src/parsing.c
        src/basic_bencode.c
        src/whole_bencode.h
        src/magnet.c
        src/file.c
        src/connection.c
        src/connection.h
)

# Link OpenSSL, CURL and Math library
target_link_libraries(bittorrent PRIVATE OpenSSL::Crypto CURL::libcurl m)

configure_file(${CMAKE_SOURCE_DIR}/test-files/big-buck-bunny.torrent ${CMAKE_BINARY_DIR}/test-files/big-buck-bunny.torrent COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/test-files/cosmos-laundromat.torrent ${CMAKE_BINARY_DIR}/test-files/cosmos-laundromat.torrent COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/test-files/sample.torrent ${CMAKE_BINARY_DIR}/test-files/sample.torrent COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/test-files/sintel.torrent ${CMAKE_BINARY_DIR}/test-files/sintel.torrent COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/test-files/tears-of-steel.torrent ${CMAKE_BINARY_DIR}/test-files/tears-of-steel.torrent COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/test-files/wired-cd.torrent ${CMAKE_BINARY_DIR}/test-files/wired-cd.torrent COPYONLY)