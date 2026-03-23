#include <stdio.h>

#include <argparse-c.h>

int main(void) {
  ap_parser *parser =
      ap_parser_new("consumer-cmake", "External CMake consumer.");
  if (!parser) {
    fprintf(stderr, "failed to create parser\n");
    return 1;
  }

  ap_parser_free(parser);
  puts("cmake consumer ok");
  return 0;
}
