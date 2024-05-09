// wrap_around.c

#include <stdio.h>


int main() {
  unsigned short start, end, delta;
  printf("sizeof(start) is %ud\n", sizeof(start));
  start = 0xfff0;
  end = 0xfff8;
  delta = end - start;
  printf("start %x, end %x, end - start %x, expect 0x08\n", start, end, delta);
  end += 0x8;
  delta = end - start;
  printf("start %x, end %x, end - start %x, expect 0x10\n", start, end, delta);
  end += 0x8;
  delta = end - start;
  printf("start %x, end %x, end - start %x, expect 0x18\n", start, end, delta);
  return 0;
}
