#include <stdio.h>
#define D 4
#include "rvn.h"

int main(void)
{
  rvn_t a = {1, 1.732}, b = {0, 0}, c = {0, 1};
  rvn_t fa, fb, fc;
  real ang, ang2, del = .001f;

  ang = rvn_ang(a, b, c, fa, fb, fc);
  printf("ang = %g\n", ang);

  rvn_sinc(a, fa, del/rvn_sqr(fa));
  ang2 = rvn_ang(a, b, c, NULL, NULL, NULL);
  printf("ang = %g, del = %g\n", ang2, ang2 - ang);

  rvn_sinc(b, fb, del/rvn_sqr(fb));
  ang = rvn_ang(a, b, c, NULL, NULL, NULL);
  printf("ang = %g, del = %g\n", ang, ang - ang2);

  rvn_sinc(c, fc, del/rvn_sqr(fc));
  ang2 = rvn_ang(a, b, c, NULL, NULL, NULL);
  printf("ang = %g, del = %g\n", ang, ang2 - ang);

  return 0;
}
