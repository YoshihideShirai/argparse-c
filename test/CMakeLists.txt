add_executable(argparse_test
  test_runner.cpp
  test_framework.cpp
  test_parse_core.cpp
  test_known_args.cpp
  test_subcommands_and_validation.cpp
  test_format_and_api.cpp
)
target_include_directories(argparse_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(argparse_test PRIVATE argparse-c)

add_test(NAME argparse_test COMMAND argparse_test -v)
