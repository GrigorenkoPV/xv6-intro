#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
  printf("Hello, world!\n");
  for (int arg = 0; arg < argc; ++arg) {
    printf("Arg #%d: \"%s\"\n", arg, argv[arg]);
  }
  printf("Goodbye, world!\n");
  exit(0);
}
