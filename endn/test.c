/* convert an integer */
#include <stdio.h>
#include "endn.h"

static void pint(int i)
{
  int n;
  unsigned char *p;

  printf("0x%08X,%12d: ", i, i);
  p = (unsigned char*)&i;
  for (n = 0; n < sizeof(int);  n++) {
    printf( "0x%02X ", p[n]);
  }
  printf("--> \n");
}

/* in-place conversion */
#define END2BIG(from) endn_converti(&(from), sizeof(from), 1, 1)
#define END2LIT(from) endn_converti(&(from), sizeof(from), 1, 0)

/* out-of-place conversion */
#define END2BIGO(to, from) endn_convert(&(to), &(from), sizeof(from), 1, 1)
#define END2LITO(to, from) endn_convert(&(to), &(from), sizeof(from), 1, 0)

int main(void)
{
  int i = 0xFEFF, j;

  printf("\n\nTesting in-place ...\n");
  i = 0xFEFF;
  printf("i before END2BIG\n");
  pint(i);
  END2BIG(i);
  printf("i after  END2BIG\n");
  pint(i);
  END2BIG(i);
  printf("i after  END2BIG again\n");
  pint(i);

  i = 0xFEFF;
  printf("\n\ni before END2LIT\n");
  pint(i);
  END2LIT(i);
  printf("i after  END2LIT\n");
  pint(i);

  printf("\n\nTesting out-of-place ...\n");
  i = 0xFEFF;
  j = 0;
  printf("\n\ni, j before END2BIGO(j, i)\n");
  pint(i);
  pint(j);
  END2BIGO(j, i);
  printf("i, j after  END2BIGO(j, i)\n");
  pint(i);
  pint(j);

  i = 0xFEFF;
  j = 0;
  printf("\n\ni, j before END2LITO(j, i)\n");
  pint(i);
  pint(j);
  END2LITO(j, i);
  printf("i, j after  END2LITO(j, i)\n");
  pint(i);
  pint(j);
  
  return 0;
}
