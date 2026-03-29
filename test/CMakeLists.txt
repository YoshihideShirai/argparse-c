add_executable(argparse_test
  test_runner.cpp
  test_framework.cpp
  test_formatters.cpp
  test_parse_core.cpp
  test_known_args.cpp
  test_subcommands_and_validation.cpp
  test_format_and_api.cpp
)
target_include_directories(argparse_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(argparse_test PRIVATE cxx_std_17)
target_compile_definitions(argparse_test PRIVATE
  AP_EXAMPLE_COMPLETION_PATH="$<TARGET_FILE:example_completion>"
  AP_EXAMPLE_MANPAGE_PATH="$<TARGET_FILE:example_manpage>"
)
target_link_libraries(argparse_test PRIVATE argparse-c ap_warnings)

add_executable(argparse_test_static
  test_runner.cpp
  test_framework.cpp
  test_formatters.cpp
  test_parse_core.cpp
  test_known_args.cpp
  test_subcommands_and_validation.cpp
  test_format_and_api.cpp
)
target_include_directories(argparse_test_static PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(argparse_test_static PRIVATE cxx_std_17)
target_compile_definitions(argparse_test_static PRIVATE
  AP_EXAMPLE_COMPLETION_PATH="$<TARGET_FILE:example_completion>"
  AP_EXAMPLE_MANPAGE_PATH="$<TARGET_FILE:example_manpage>"
)
target_link_libraries(argparse_test_static PRIVATE argparse-c-static ap_warnings)

add_test(NAME argparse_test COMMAND argparse_test -v)
add_test(NAME argparse_test_static COMMAND argparse_test_static -v)
