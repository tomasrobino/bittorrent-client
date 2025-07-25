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
        src/downloading.c
        src/downloading.h
        src/messages.c
        src/messages.h
        src/file.h
)

# Link OpenSSL, CURL and Math library
target_link_libraries(bittorrent PRIVATE OpenSSL::Crypto CURL::libcurl m)

configure_file("${CMAKE_SOURCE_DIR}/test-files/Antro [FitGirl Repack].torrent" "${CMAKE_BINARY_DIR}/test-files/Antro [FitGirl Repack].torrent" COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/test-files/big-buck-bunny.torrent ${CMAKE_BINARY_DIR}/test-files/big-buck-bunny.torrent COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/test-files/cosmos-laundromat.torrent ${CMAKE_BINARY_DIR}/test-files/cosmos-laundromat.torrent COPYONLY)
configure_file("${CMAKE_SOURCE_DIR}/test-files/Dragons Reef [FitGirl Repack].torrent" "${CMAKE_BINARY_DIR}/test-files/Dragons Reef [FitGirl Repack].torrent" COPYONLY)
configure_file("${CMAKE_SOURCE_DIR}/test-files/LIMINAL SHIFT [FitGirl Repack].torrent" "${CMAKE_BINARY_DIR}/test-files/LIMINAL SHIFT [FitGirl Repack].torrent" COPYONLY)
configure_file("${CMAKE_SOURCE_DIR}/test-files/Parcel Simulator [FitGirl Repack].torrent" "${CMAKE_BINARY_DIR}/test-files/Parcel Simulator [FitGirl Repack].torrent" COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/test-files/sample.torrent ${CMAKE_BINARY_DIR}/test-files/sample.torrent COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/test-files/sintel.torrent ${CMAKE_BINARY_DIR}/test-files/sintel.torrent COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/test-files/tears-of-steel.torrent ${CMAKE_BINARY_DIR}/test-files/tears-of-steel.torrent COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/test-files/torrentdyne_tolerant_goblin_http_ipv4.probe.torrent ${CMAKE_BINARY_DIR}/test-files/torrentdyne_tolerant_goblin_http_ipv4.probe.torrent COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/test-files/torrentdyne_tolerant_goblin_http_ipv6.probe.torrent ${CMAKE_BINARY_DIR}/test-files/torrentdyne_tolerant_goblin_http_ipv6.probe.torrent COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/test-files/wired-cd.torrent ${CMAKE_BINARY_DIR}/test-files/wired-cd.torrent COPYONLY)