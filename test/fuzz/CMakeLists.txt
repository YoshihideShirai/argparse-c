if(NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
  message(FATAL_ERROR "AP_ENABLE_FUZZING requires Clang (libFuzzer)")
endif()

add_executable(argparse_fuzz_harness
  fuzz_harness.c
)

target_link_libraries(argparse_fuzz_harness PRIVATE argparse-c-static ap_warnings)

target_compile_options(argparse_fuzz_harness PRIVATE
  -O1
  -g
  -fno-omit-frame-pointer
  -fsanitize=fuzzer,address,undefined
)

target_link_options(argparse_fuzz_harness PRIVATE
  -fsanitize=fuzzer,address,undefined
)

add_custom_target(fuzz-smoke
  COMMAND $<TARGET_FILE:argparse_fuzz_harness>
    -max_total_time=30
    -rss_limit_mb=2048
    ${CMAKE_CURRENT_SOURCE_DIR}/corpus
  USES_TERMINAL
  COMMENT "Running libFuzzer smoke test"
)

add_custom_target(fuzz-long
  COMMAND $<TARGET_FILE:argparse_fuzz_harness>
    -max_total_time=300
    -rss_limit_mb=4096
    ${CMAKE_CURRENT_SOURCE_DIR}/corpus
  USES_TERMINAL
  COMMENT "Running extended libFuzzer job"
)
