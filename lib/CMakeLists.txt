add_library(argparse-c SHARED
  api.c
  core_parser.c
  core_validate.c
  core_convert.c
  format_help.c
)

add_library(argparse-c::argparse-c ALIAS argparse-c)

target_include_directories(argparse-c
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(argparse-c PUBLIC c_std_99)

if(CMAKE_C_COMPILER_ID MATCHES "Clang")
  target_compile_options(argparse-c PRIVATE -Wall -Wextra -Wpedantic)
endif()

install(TARGETS argparse-c
  EXPORT argparse-c-targets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES ${CMAKE_SOURCE_DIR}/include/argparse-c.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT argparse-c-targets
  FILE argparse-c-targets.cmake
  NAMESPACE argparse-c::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/argparse-c
)
