cmake_minimum_required(VERSION 3.16)
project(argparse-c C CXX)

option(AP_ENABLE_COVERAGE "Enable coverage instrumentation for gcovr" OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)

if(AP_ENABLE_COVERAGE)
  if(NOT CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    message(FATAL_ERROR "AP_ENABLE_COVERAGE requires GCC or Clang")
  endif()

  add_compile_options(-O0 -g --coverage)
  add_link_options(--coverage)

  set(AP_GCOV_EXECUTABLE gcov)
  if(CMAKE_C_COMPILER_ID MATCHES "Clang")
    set(AP_GCOV_EXECUTABLE "llvm-cov gcov")
  endif()
endif()

include_directories(./include)
add_subdirectory(lib)
add_subdirectory(sample)

enable_testing()
add_subdirectory(test)

set(AP_CLANG_TOOL_SOURCES
  include/argparse-c.h
  lib/api.c
  lib/ap_internal.h
  lib/core_convert.c
  lib/core_parser.c
  lib/core_validate.c
  lib/format_help.c
  lib/format_completion_fish.c
  sample/example1.c
  sample/example_subcommands.c
  test/test_format_and_api.cpp
  test/test_framework.cpp
  test/test_framework.h
  test/test_known_args.cpp
  test/test_parse_core.cpp
  test/test_runner.cpp
  test/test_subcommands_and_validation.cpp
)

find_program(AP_CLANG_FORMAT_EXECUTABLE clang-format)
if(AP_CLANG_FORMAT_EXECUTABLE)
  add_custom_target(format
    COMMAND ${AP_CLANG_FORMAT_EXECUTABLE} -i
      ${AP_CLANG_TOOL_SOURCES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    USES_TERMINAL
    COMMENT "Formatting sources with clang-format"
  )
  add_custom_target(format-check
    COMMAND ${AP_CLANG_FORMAT_EXECUTABLE} --dry-run --Werror
      ${AP_CLANG_TOOL_SOURCES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    USES_TERMINAL
    COMMENT "Checking source formatting with clang-format"
  )
else()
  message(STATUS "clang-format not found; 'format' and 'format-check' targets will be unavailable")
endif()

find_program(AP_CLANG_TIDY_EXECUTABLE clang-tidy)
if(AP_CLANG_TIDY_EXECUTABLE)
  add_custom_target(tidy
    COMMAND ${AP_CLANG_TIDY_EXECUTABLE}
      -p ${CMAKE_BINARY_DIR}
      ${AP_CLANG_TOOL_SOURCES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    USES_TERMINAL
    COMMENT "Running clang-tidy checks"
  )
else()
  message(STATUS "clang-tidy not found; 'tidy' target will be unavailable")
endif()

find_package(Python3 COMPONENTS Interpreter)

if(AP_ENABLE_COVERAGE AND Python3_Interpreter_FOUND)
  add_custom_target(coverage
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    COMMAND ${Python3_EXECUTABLE} -m gcovr
      --root ${CMAKE_SOURCE_DIR}
      --filter ${CMAKE_SOURCE_DIR}/lib
      --object-directory ${CMAKE_BINARY_DIR}
      --gcov-executable ${AP_GCOV_EXECUTABLE}
      --txt coverage.txt
      --html-details coverage.html
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    USES_TERMINAL
    COMMENT "Running tests and generating gcovr coverage reports"
  )
endif()
