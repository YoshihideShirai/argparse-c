set(AP_ARGPARSE_C_SOURCES
  api.c
  core_parser.c
  core_validate.c
  core_convert.c
  format_help.c
  format_manpage.c
  format_completion_bash.c
  format_completion_fish.c
  format_completion_zsh.c
)

add_library(argparse-c SHARED ${AP_ARGPARSE_C_SOURCES})
add_library(argparse-c-static STATIC ${AP_ARGPARSE_C_SOURCES})

set_target_properties(argparse-c PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
  EXPORT_NAME argparse-c
  OUTPUT_NAME argparse-c
)

set_target_properties(argparse-c-static PROPERTIES
  EXPORT_NAME argparse-c-static
  OUTPUT_NAME argparse-c
)

add_library(argparse-c::argparse-c ALIAS argparse-c)
add_library(argparse-c::argparse-c-static ALIAS argparse-c-static)

foreach(ap_target IN ITEMS argparse-c argparse-c-static)
  target_include_directories(${ap_target}
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )

  target_compile_features(${ap_target} PUBLIC c_std_99)

  if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${ap_target} PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endforeach()

install(TARGETS argparse-c argparse-c-static
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
