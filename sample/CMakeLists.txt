add_executable(example1 example1.c)
target_link_libraries(example1 argparse-c)

add_executable(example_subcommands example_subcommands.c)
target_link_libraries(example_subcommands argparse-c)

add_executable(example_completion example_completion.c)
target_link_libraries(example_completion argparse-c)

add_executable(example_manpage example_manpage.c)
target_link_libraries(example_manpage argparse-c)

add_executable(example_introspection example_introspection.c)
target_link_libraries(example_introspection argparse-c)
