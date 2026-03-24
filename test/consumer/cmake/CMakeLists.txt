cmake_minimum_required(VERSION 3.16)
project(argparse_c_consumer_cmake LANGUAGES C)

find_package(argparse-c CONFIG REQUIRED)

add_executable(argparse_c_consumer main.c)
target_link_libraries(argparse_c_consumer PRIVATE argparse-c::argparse-c)
target_compile_features(argparse_c_consumer PRIVATE c_std_99)
