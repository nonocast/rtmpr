#include <stdio.h>

int main() {
  char name[] = "nonocast";
  char *another = name;
  printf("%s\n", another);
  another[0] = '0x00';

  return 0;
}