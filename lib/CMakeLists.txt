add_library(argparse-c SHARED
  api.c
  core_parser.c
  core_validate.c
  core_convert.c
  format_help.c
)

target_include_directories(argparse-c PUBLIC ${CMAKE_SOURCE_DIR}/include)

target_compile_features(argparse-c PUBLIC c_std_99)

if(CMAKE_C_COMPILER_ID MATCHES "Clang")
  target_compile_options(argparse-c PRIVATE -Wall -Wextra -Wpedantic)
endif()
