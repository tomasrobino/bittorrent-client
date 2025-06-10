cmake_minimum_required(VERSION 3.13)

project(bittorrent-client)

file(GLOB_RECURSE SOURCE_FILES src/*.c src/*.h)

set(CMAKE_C_STANDARD 23) # Enable the C23 standard

find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)

add_executable(bittorrent ${SOURCE_FILES}
        src/structs.h
        src/parsing.c
        src/parsing.h
        src/basic_bencode.c
        src/basic_bencode.h)

# Link OpenSSL, CURL and Math library
target_link_libraries(bittorrent PRIVATE OpenSSL::Crypto CURL::libcurl m)

configure_file(${CMAKE_SOURCE_DIR}/big-buck-bunny.torrent ${CMAKE_BINARY_DIR}/big-buck-bunny.torrent COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/cosmos-laundromat.torrent ${CMAKE_BINARY_DIR}/cosmos-laundromat.torrent COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/sample.torrent ${CMAKE_BINARY_DIR}/sample.torrent COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/sintel.torrent ${CMAKE_BINARY_DIR}/sintel.torrent COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/tears-of-steel.torrent ${CMAKE_BINARY_DIR}/tears-of-steel.torrent COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/wired-cd.torrent ${CMAKE_BINARY_DIR}/wired-cd.torrent COPYONLY)