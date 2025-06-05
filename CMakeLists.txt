cmake_minimum_required(VERSION 3.13)

project(bittorrent-client)

file(GLOB_RECURSE SOURCE_FILES src/*.c src/*.h)

set(CMAKE_C_STANDARD 23) # Enable the C23 standard

find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)

add_executable(bittorrent ${SOURCE_FILES})

# Link OpenSSL and CURL
target_link_libraries(bittorrent PRIVATE OpenSSL::Crypto CURL::libcurl)
