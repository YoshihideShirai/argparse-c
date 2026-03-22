find_package(PkgConfig REQUIRED)
pkg_check_modules(CPPUTEST REQUIRED cpputest)

add_executable(argparse_test argparse_test.cpp)
target_include_directories(argparse_test PRIVATE ${CPPUTEST_INCLUDE_DIRS})
target_link_libraries(argparse_test PRIVATE argparse-c ${CPPUTEST_LIBRARIES})
target_link_directories(argparse_test PRIVATE ${CPPUTEST_LIBRARY_DIRS})
target_compile_options(argparse_test PRIVATE ${CPPUTEST_CFLAGS_OTHER})

add_test(NAME argparse_test COMMAND argparse_test -v)
