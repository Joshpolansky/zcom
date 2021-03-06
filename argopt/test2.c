#include "argopt.h"

#define ARGOPT_DELIM ':'

/* register an option/argument in the shortcut formt
 * maybe not so intuitive */
INLINE int argopt_reg(argopt_t *ao, const char *str, void *ptr)
{
  char *fmt, *desc, *opt;

  xnew(opt, strlen(str) + 1);
  strcpy(opt, str);
  if (*opt == '-') { /* options */
    die_if ((fmt = strchr(opt, ARGOPT_DELIM)) == NULL, "bad input [%s]\n", opt);
    *fmt++ = '\0';
    if ((desc = strchr(fmt, ARGOPT_DELIM)) != NULL) *desc++ = '\0';
    return argopt_regopt(ao, opt, fmt, ptr, desc);
  } else {
    if ((desc = strchr(fmt = opt, ARGOPT_DELIM)) != NULL) *desc++ = '\0';
    return argopt_regarg(ao, fmt, ptr, desc);
  }
}



int main(int argc, char **argv)
{
  argopt_t *ao;
  const char *fn = NULL, *fn2 = NULL;
  int n = 0, freq = -1, verbose = 0;
  double x = 1e-31;
  real y = 0;
  int nset, xset, yset, fset;

  ao = argopt_open(0);
  argopt_reg(ao, "!:inputfile", &fn);
  argopt_reg(ao, ":inputfile2", &fn2);
  argopt_reg(ao, "-h:%b:$HELP", &ao->dum_);
  argopt_reg(ao, "--version:%b:$VERSION", &ao->dum_);
  argopt_reg(ao, "--verbose:%b:verbose", &verbose);
  argopt_reg(ao, "--freq:%d:frequency of saving files", &freq);
  argopt_reg(ao, "-n:!%d:an integer n", &n);
  argopt_reg(ao, "-x:%lf:a double x", &x);
  argopt_reg(ao, "-y:%r:a real y", &y);
  argopt_parse(ao, argc, argv);
  nset = argopt_isset(ao, n);
  xset = argopt_isset(ao, x);
  yset = argopt_isset(ao, y);
  fset = argopt_isset(ao, freq);
  printf("fn %s, fn2 %s, verbose %d, n[%d] %d, x[%d] %g, y[%d] %g, freq[%d] %d\n",
      fn, fn2, verbose, nset,  n, xset, x, yset, y, fset, freq);
  argopt_close(ao);
  return 0;
}
