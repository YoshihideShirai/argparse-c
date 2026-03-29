cmake_minimum_required(VERSION 3.16)
project(argparse-c VERSION 0.1.0 LANGUAGES C CXX)

option(AP_ENABLE_COVERAGE "Enable coverage instrumentation for gcovr" OFF)
option(AP_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(AP_ENABLE_FUZZING "Enable libFuzzer harness build" OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

include(${CMAKE_SOURCE_DIR}/cmake/APWarnings.cmake)

set(AP_CMAKE_PACKAGE_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/argparse-c")

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

if(AP_ENABLE_SANITIZERS)
  if(NOT CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    message(FATAL_ERROR "AP_ENABLE_SANITIZERS requires GCC or Clang")
  endif()

  set(AP_SANITIZER_FLAGS
    -fsanitize=address
    -fsanitize=undefined
    -fno-omit-frame-pointer
  )

  add_compile_options(-O1 -g ${AP_SANITIZER_FLAGS})
  add_link_options(${AP_SANITIZER_FLAGS})
endif()

configure_package_config_file(
  ${CMAKE_SOURCE_DIR}/cmake/argparse-cConfig.cmake.in
  ${CMAKE_BINARY_DIR}/argparse-cConfig.cmake
  INSTALL_DESTINATION ${AP_CMAKE_PACKAGE_DIR}
)

write_basic_package_version_file(
  ${CMAKE_BINARY_DIR}/argparse-cConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

configure_file(
  ${CMAKE_SOURCE_DIR}/argparse-c.pc.in
  ${CMAKE_BINARY_DIR}/argparse-c.pc
  @ONLY
)

include_directories(./include)
add_subdirectory(lib)
add_subdirectory(sample)

enable_testing()
add_subdirectory(test)
if(AP_ENABLE_FUZZING)
  add_subdirectory(test/fuzz)
endif()

install(FILES
  ${CMAKE_BINARY_DIR}/argparse-cConfig.cmake
  ${CMAKE_BINARY_DIR}/argparse-cConfigVersion.cmake
  DESTINATION ${AP_CMAKE_PACKAGE_DIR}
)

install(FILES
  ${CMAKE_BINARY_DIR}/argparse-c.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

set(AP_CLANG_TOOL_SOURCES
  include/argparse-c.h
  lib/ap_internal.h
  ${AP_ARGPARSE_C_TOOL_SOURCES}
  sample/example1.c
  sample/example_subcommands.c
  sample/consumer/cmake/main.c
  sample/consumer/pkg-config/main.c
  test/test_format_and_api.cpp
  test/test_formatters.cpp
  test/test_framework.cpp
  test/test_framework.h
  test/test_alloc_injector.cpp
  test/test_known_args.cpp
  test/test_parse_core.cpp
  test/test_runner.cpp
  test/test_subcommands_and_validation.cpp
  test/fuzz/fuzz_harness.c
)

set(AP_CLANG_TIDY_SOURCES
  lib/ap_internal.h
  ${AP_ARGPARSE_C_TOOL_SOURCES}
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
      ${AP_CLANG_TIDY_SOURCES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    USES_TERMINAL
    COMMENT "Running clang-tidy checks"
  )
else()
  message(STATUS "clang-tidy not found; 'tidy' target will be unavailable")
endif()

find_package(Python3 COMPONENTS Interpreter)

if(Python3_Interpreter_FOUND)
  add_custom_target(sync-docs-repository
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/sync_docs_repository.py
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    USES_TERMINAL
    COMMENT "Synchronizing docs/repository snapshots"
  )
else()
  message(STATUS "Python3 not found; 'sync-docs-repository' target will be unavailable")
endif()

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
