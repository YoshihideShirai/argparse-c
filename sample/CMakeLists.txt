add_executable(example1 example1.c)
target_link_libraries(example1 argparse-c)

add_executable(example_subcommands example_subcommands.c)
target_link_libraries(example_subcommands argparse-c)
