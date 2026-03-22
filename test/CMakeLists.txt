add_executable(argparse_test argparse_test.cpp)
target_include_directories(argparse_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(argparse_test PRIVATE argparse-c)

add_test(NAME argparse_test COMMAND argparse_test -v)
