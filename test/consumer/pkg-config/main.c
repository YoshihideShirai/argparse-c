#include <stdio.h>

#include <argparse-c.h>

int main(void) {
  ap_parser *parser =
      ap_parser_new("consumer-pkg-config", "External pkg-config consumer.");
  if (!parser) {
    fprintf(stderr, "failed to create parser\n");
    return 1;
  }

  ap_parser_free(parser);
  puts("pkg-config consumer ok");
  return 0;
}
