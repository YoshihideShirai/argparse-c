cmake_minimum_required(VERSION 3.16)
project(argparse-c C CXX)

option(AP_ENABLE_COVERAGE "Enable coverage instrumentation for gcovr" OFF)

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

find_package(Python3 COMPONENTS Interpreter)

if(AP_ENABLE_COVERAGE AND Python3_Interpreter_FOUND)
  add_custom_target(coverage
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    COMMAND ${Python3_EXECUTABLE} -m gcovr
      --root ${CMAKE_SOURCE_DIR}
      --filter ${CMAKE_SOURCE_DIR}
      --object-directory ${CMAKE_BINARY_DIR}
      --gcov-executable ${AP_GCOV_EXECUTABLE}
      --exclude '.*/test/.*'
      --exclude '.*/sample/.*'
      --txt coverage.txt
      --html-details coverage.html
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    USES_TERMINAL
    COMMENT "Running tests and generating gcovr coverage reports"
  )
endif()
