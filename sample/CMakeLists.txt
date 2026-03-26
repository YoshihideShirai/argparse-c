add_executable(example1 example1.c)
target_link_libraries(example1 PRIVATE argparse-c ap_warnings)

add_executable(example_subcommands example_subcommands.c)
target_link_libraries(example_subcommands PRIVATE argparse-c ap_warnings)

add_executable(example_completion example_completion.c)
target_link_libraries(example_completion PRIVATE argparse-c ap_warnings)

add_executable(example_manpage example_manpage.c)
target_link_libraries(example_manpage PRIVATE argparse-c ap_warnings)

add_executable(example_introspection example_introspection.c)
target_link_libraries(example_introspection PRIVATE argparse-c ap_warnings)
