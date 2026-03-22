add_executable(argparse_test argparse_test.c)
target_link_libraries(argparse_test argparse-c)

add_test(NAME argparse_test COMMAND argparse_test)
