cmake_minimum_required(VERSION 3.13)
set(CMAKE_C_STANDARD 23)
include(FetchContent)

FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.5.2
)

FetchContent_MakeAvailable(Catch2)

file(GLOB_RECURSE SOURCE_FILES ../src/*.c ../src/*.h)

# Exclude main.c if it's in src/
list(FILTER SOURCE_FILES EXCLUDE REGEX ".*/main\\.c$")

add_executable(tests
        tests.cpp
        ${SOURCE_FILES}
)

find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)

target_include_directories(tests PRIVATE ../src)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain OpenSSL::Crypto CURL::libcurl m)

include(CTest)
include(Catch)
catch_discover_tests(tests)
