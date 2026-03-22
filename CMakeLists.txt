cmake_minimum_required(VERSION 3.16)
project(argparse-c C)

include_directories(./include)
add_subdirectory(lib)
add_subdirectory(sample)

enable_testing()
add_subdirectory(test)
