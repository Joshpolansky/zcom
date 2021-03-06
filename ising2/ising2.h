#include "rng.h"
#include "util.h"
#include "rng.h"
#ifndef ISING2_H__
#define ISING2_H__
/* two-dimensional Ising model */


typedef struct {
  int d, l, n;
  int M, E;
  int *s; /* 0 or 1 */
  double *logdos; /* logarithmic density of states */
  /* helper vars */
  uint32_t *uproba; /* temporary probability for MC transitions */
  /* for Wolff's algorithm */
  int *queue;
  char *used;
} ising_t;



/* initialize an lxl Ising model */
INLINE ising_t *is2_open(int l)
{
  int i, n;
  ising_t *is;

  xnew(is, 1);
  is->d = 2;
  is->l = l;
  is->n = n = l*l;
  xnew(is->s, n);
  for (i = 0; i < n; i++) is->s[i] = -1;
  is->M = -n;
  is->E = -2*n;
  xnew(is->logdos, n + 1);
  xnew(is->uproba, 2*is->d + 1);
  is->uproba[0] = 0xffffffff;
  xnew(is->queue, n);
  xnew(is->used, n);
  return is;
}



INLINE void is2_close(ising_t *is)
{
  if (is != NULL) {
    free(is->s);
    free(is->logdos);
    free(is->uproba);
    free(is->queue);
    free(is->used);
    free(is);
  }
}



/* set transition probability */
#define IS2_SETPROBA(is, bet) { \
  double x_ = exp(-4. * bet); \
  is->uproba[2] = (uint32_t) ((double)(0xffffffff) * x_); \
  is->uproba[4] = (uint32_t) ((double)(0xffffffff) * x_*x_); }



/* compute total energy and magnetization */
INLINE int is2_em(ising_t *is)
{
  int l, n, i, j, e, m;

  e = m = 0;
  l = is->l;
  n = l * l;
  for ( i = 0; i < n; i += l ) {
    for ( j = 0; j < l; j++ ) {
      int id = i + j;
      int idr = i + (j + 1) % l;
      int idu = (i + l) % n + j;
      int s = is->s[id];
      int su = is->s[idu];
      int sr = is->s[idr];
      m += s;
      e += s * (su + sr);
    }
  }
  is->M = m;
  return is->E = -e;
}



INLINE int is2_check(ising_t *is)
{
  int i, e, m;

  for (i = 0; i < is->n; i++)
    if (is->s[i] != 1 && is->s[i] != -1) {
      fprintf(stderr, "error: s[%d] = %d\n", i, is->s[i]);
      return -1;
    }
  e = is->E;
  m = is->M;
  is2_em(is);
  if  (e != is->E || e < -2*is->n || e > 2*is->n
    || m != is->M || m < -is->n   || m > is->n) {
    fprintf(stderr, "error: E = %d, %d; M = %d, %d\n",
        e, is->E, m, is->M);
    return -1;
  }
  return 0;
}



/* pick a random site, count neighbors with different spins */
INLINE int is2_pick(const ising_t *is, int *h)
{
  int id, ix, iy, l, lm, n, nm, ssn;

  lm = (l = is->l) - 1;
  nm = (n = is->n) - l;
  id = (int) (rand01() * n);
  ix = id % l;
  iy = id / l;
  ssn = ((ix != 0 ) ? is->s[id - 1] : is->s[id + lm])   /* left  */
      + ((ix != lm) ? is->s[id + 1] : is->s[id - lm])   /* right */
      + ((iy != 0 ) ? is->s[id - l] : is->s[id + nm])   /* down  */
      + ((iy != lm) ? is->s[id + l] : is->s[id - nm]);  /* up    */
  *h = is->s[id] * ssn; /* -(*h) is the energy before the flip */
  return id;
}



/* flip site id, with h different neighbors */
INLINE int is2_flip(ising_t *is, int id, int h)
{
  die_if(id >= is->n, "bad id %d/%d\n", id, is->n);
  is->M += (is->s[id] = -is->s[id])*2;
  return is->E += h*2;
}



/* faster macros for systems with fixed (upon compiling) size
 * to use them one must define IS2_LB before including
 * IS2_PICK()/IS2_PSEQ() and IS2_FLIP() */
#ifdef  IS2_LB  /* L = 2^LB, N = L*L */
#define IS2_L   (1 << IS2_LB)
#define IS2_N   (IS2_L * IS2_L)

#define IS2_GETH(is, id, h) { \
  unsigned ix, ixp, ixm, iy, iyp, iym; \
  ix = id % IS2_L; \
  iy = id - ix; \
  ixp = (ix + 1) % IS2_L; \
  ixm = (ix + (IS2_L - 1)) % IS2_L; \
  iyp = (iy + IS2_L) % IS2_N; \
  iym = (iy + (IS2_N - IS2_L)) % IS2_N; \
  h = is->s[id] * ( is->s[iy  + ixp] \
                  + is->s[iy  + ixm] \
                  + is->s[iyp + ix ] \
                  + is->s[iym + ix ] ); }
#define IS2_IRND(is, id)  id = rand32() >> (32 - 2*IS2_LB);
/* random picking */
#define IS2_PICK(is, id, h) { IS2_IRND(is, id); IS2_GETH(is, id, h); }
#define IS2_ISEQ(is, id)  id = (id + 1) % IS2_N;
/* sequential picking */
#define IS2_PSEQ(is, id, h) { IS2_ISEQ(is, id); IS2_GETH(is, id, h); }

#define IS2_FLIP(is, id, h) { \
  is->M += (is->s[id] = -is->s[id]) * 2; \
  is->E += h * 2; }

#else

#define IS2_PICK(is, id, h)  id = is2_pick(is, &h)
#define IS2_FLIP(is, id, h)  is2_flip(is, id, h)

#endif



/* add spin j to the queue if s[j] is different from s
 * return the spin */
INLINE int is2_addtoqueue(ising_t *is, int j, int s,
    double r, int *cnt)
{
  int sj = is->s[j];

  if ( sj == s && !is->used[j] && rand01() < r ) {
    is->queue[ (*cnt)++ ] = j;
    is->used[j] = (char) 1;
  }
  return sj;
}



/* Wolff algorithm */
INLINE int is2_wolff(ising_t *is, double padd)
{
  int l = is->l, n = is->n, i, ix, iy, id, s, cnt = 0, h = 0;

  /* randomly selected a seed */
  id = (int) ( rand01() * n );
  s = is->s[id];
  is->queue[ cnt++ ] = id;
  for ( i = 0; i < n; i++ ) {
    is->used[i] = 0;
  }
  is->used[id] = (char) 1;

  /* go through spins in the queue */
  for ( i = 0; i < cnt; i++ ) {
    id = is->queue[i];
    /* flip the spin to correctly compute the local field,
     * which is the total magnetization of all spins
     * surrounding the cluster.
     *
     * consider a bond id-jd, with jd being a neighbor of id
     * 1) if jd does not make it to the cluster, then it
     *    lies on the border, and it contributes
     *    s[jd] to the local field
     * 2) if s[jd] == s, and will be included in the cluster
     *    in the future, it should contribute zero to the
     *    local field.  But we let it contribute s to the
     *    local field for now.  Since jd is added to the
     *    queue, when jd is considered in this loop, or
     *    when the bond jd-id is reconsidered, it will
     *    contribute an s[id] to the local field.  But at
     *    that time, s[id] = -s due to the flip here,
     *    so the total contribution would be s + (-s) = 0.
     *  */
    is->s[id] = -s;
    /* add neighbors of i with the same spins */
    ix = id % l;
    iy = id - ix;
    h += is2_addtoqueue(is, iy + (ix + 1) % l,     s, padd, &cnt);
    h += is2_addtoqueue(is, iy + (ix + l - 1) % l, s, padd, &cnt);
    h += is2_addtoqueue(is, (iy + l) % n + ix,     s, padd, &cnt);
    h += is2_addtoqueue(is, (iy + n - l) % n + ix, s, padd, &cnt);
  }

  is->E += 2 * s * h;
  is->M -= 2 * s * cnt;
  return 0;
}



INLINE int is2_save(const ising_t *is, const char *fname)
{
  FILE *fp;
  int i, j, l, *p;

  xfopen(fp, fname, "w", return -1);
  l = is->l;
  fprintf(fp, "%d %d %d %d\n", is->d, l, l, is->n);
  for (p = is->s, i = 0; i < l; i++) {
    for (j = 0; j < l; j++, p++)
      fprintf(fp, "%c", (*p > 0) ? '#' : ' ');
    fprintf(fp, "\n");
  }
  fclose(fp);
  return 0;
}



INLINE int is2_load(ising_t *is, const char *fname)
{
  FILE *fp;
  int i, lx, ly, n, c;
  char s[80];

  xfopen(fp, fname, "r", return -1);
  if (fgets(s, sizeof s, fp) == NULL) {
    fprintf(stderr, "missing first line %s\n", fname);
    fclose(fp);
    return -1;
  }
  if (4 != sscanf(s, "%d%d%d%d", &i, &lx, &ly, &n)
      || i != 2 || lx != ly || lx != is->l || n != is->n) {
    fprintf(stderr, "bad setting: %dD, %dx%d = %d\n", i, lx, ly, n);
    return -1;
  }
  for (i = 0; i < n; i++) {
    while ((c = fgetc(fp)) != EOF && c == '\n') ;
    if (c == EOF) break;
    is->s[i] = (c == ' ') ? -1 : 1;
  }
  if (i < n)
    fprintf(stderr, "%s: data stopped at i = %d\n", fname, i);
  fclose(fp);
  is2_em(is);
  return 0;
}



/* exact solution of ising model */
INLINE double is2_exact(ising_t *is, double beta, double *eav, double *cv)
{
  double lxh, n, ex, f, th, sech, bet2, bsqr, log2, x;
  double lnz, lnz1, lnz2, lnz3, lnz4, dz, ddz;
  double z21, z31, z41, za1;
  double dr1, dr2, dr3, dr4, ddr1, ddr2, ddr3, ddr4;
  double g, g0, dg, ddg, dg0;
  double xn2b, sh2b, coth2b;
  double lnch2b, lncc2b, lncl, lnsl, cd, cdsqr, lnddcl;
  int r, sgn4 = 1, lx, ly;

  lx = is->l;
  ly = is->l;
  lxh = .5*lx;
  n = lx * ly;
  log2 = log(2.0);
  bet2 = 2.*beta;
  bsqr = beta*beta;
  xn2b = exp(-bet2);
  if (lx == 2 && ly == 2) { /* 2x2 system */
    double lnc, lnd;
    x = 8.*beta;
    lnc = lnadd(x, -x);
    lnd = lnaddn(lnc, 6.);
    lnz = lnd + log2;
    if (eav) *eav = -8.*exp(lndif(x, -x) - lnd); /* -8*sinh(8*b)/(3+cosh(8*h)) */
    if (cv) *cv = bsqr * 384. * exp(lnaddn(lnc, 2./3) - 2.0*lnd); /* 64*(1+3cosh(8*b))/(3+cosh(8*b))^2 */
    return lnz;
  } else if (fabs(beta) < 1e-6) { /* high T approx. normal branch unstable if beta < 1e-6 */
    lnz = n * (2.*lnadd(beta, -beta) - log2);
    x = 1. + xn2b;
    if (eav) *eav = -2. * n * (1. - xn2b)/x;
    if (cv) *cv = bsqr * 8.*n*xn2b/(x*x);
    return lnz; /* +n*tanh(beta)^4 */
  }

  lnz1 = lnz2 = lnz3 = lnz4 = 0;
  dr1 = dr2 = dr3 = dr4 = 0;
  ddr1 = ddr2 = ddr3 = ddr4 = 0;
  lnch2b = lnadd(bet2, -bet2) - log2;
  coth2b = 2./(1. - xn2b*xn2b) - 1.;
  lncc2b = lnch2b + log(coth2b); /* ln[ cosh(2b) * coth(2b) ] */
  g0 = bet2 + log(2./(1. + xn2b) - 1.);
  sgn4 = (g0 >= 0) ? 1 : -1;

  sh2b = 0.5*(1./xn2b - xn2b);
  dg0 = 2. + 2./sh2b;
  x = sh2b*sh2b;
  cd = 2. - 2./x; /* cl' = cd * cosh(2b) */
  cdsqr = cd*cd;
  lnddcl = lnaddn(lncc2b, 2.0/(x * sh2b)) + 2.*log2; /* log(cl'') */

  for (r = 0; r < ly; r++) { /* for odd number */
    lncl = lnaddn(lncc2b, -cos((2.*r + 1.)*M_PI/ly));
    lnsl = lncl + 0.5*log(1. - exp(-2.*lncl));
    g = lnadd(lncl, lnsl);
    f = lxh*g;
    lnz1 += lnadd(f, -f);
    lnz2 += lndif(f, -f);

    dg = exp(lnch2b - lnsl)*cd; /* g' = cl'/sl; */
    ex = exp(-f);
    th = 2./(1. + ex*ex) - 1.;
    x = lxh*dg;
    dr1 += x*th;
    dr2 += x/th;

    /* g''=cl''/sl - cl' ^2 *cl/sl^3; */
    ddg = exp(lnddcl - lnsl);
    ddg -= exp(lnch2b*2. + lncl - 3.*lnsl)*cdsqr;
    sech = 2.0*dg/(ex + 1.0/ex); /* g' * sech(0.5*lx*g) */
    ddr1 += lxh*(ddg*th + lxh*(sech*sech));
    sech = 2.0*dg/(ex - 1.0/ex); /* g' * sech(0.5*lx*g) */
    ddr2 += lxh*(ddg/th - lxh*(sech*sech));

    if (r == 0) {
      g = g0;
    } else {
      lncl = lnaddn(lncc2b, -cos(2.0*M_PI*r/ly));
      lnsl = lncl+0.5*log(1-exp(-2*lncl));
      g = lnadd(lncl, lnsl);
      die_if (g < 0.0, "g = %g < 0.\n", g);;
    }
    f = lxh*g;
    lnz3 += lnadd(f, -f); /* log [2 cosh(f)] */
    lnz4 += (f < 0) ? lndif(-f, f) : lndif(f, -f); /* avoid neg. g0 */

    ex = exp(-f);
    th = 2./(1. + ex*ex) - 1.;
    dg = (r == 0) ? dg0 : exp(lnch2b - lnsl)*cd;
    dr3 += lxh*dg*th;
    dr4 += lxh*dg/th;

    if (r == 0) {
      ddg = -4*coth2b*coth2b*exp(-lnch2b);
    } else {
      ddg = exp(lnddcl - lnsl);
      ddg -= exp(lnch2b*2. + lncl - 3.*lnsl)*cdsqr;
    }
    sech = 2.0*dg/(ex + 1.0/ex);
    ddr3 += lxh*(ddg*th + lxh*(sech*sech));
    sech = 2.0*dg/(ex - 1.0/ex);
    ddr4 += lxh*(ddg/th - lxh*(sech*sech));
  }

  z21 = exp(lnz2 - lnz1);
  z31 = exp(lnz3 - lnz1);
  z41 = sgn4*exp(lnz4 - lnz1);
  za1 = 1.0 + z21 + z31 + z41;
  lnz = lnz1 + log(za1);
  lnz += .5*n*log(2.*sh2b) - log2;
  dz = (dr1 + z21*dr2 + z31*dr3 + z41*dr4)/za1;
  if (eav) *eav = - n*coth2b - dz;
  ddr1 += dr1*dr1;
  ddr2 += dr2*dr2;
  ddr3 += dr3*dr3;
  ddr4 += dr4*dr4;
  ddz = (ddr1 + z21*ddr2 + z31*ddr3 + z41*ddr4)/za1;
  if (cv) *cv = bsqr * (-2.*n/(sh2b*sh2b) + ddz - dz*dz);
  return lnz;
}



#define is2_loadlogdos(is, fn) \
  is2loadlogdos(is->logdos, is->l, is->l, fn)

/* load the exact logarithmic density of states from file
 * both n and m should be is->l
 * Minimal Mathematica script to generate exact DOS files

NDOS[m_, n_] := Module[{x, xp, prec = Floor[1.5 n m Log[2]/Log[10]], b, a, c2, s2, c0, s0, cn, sn},
  b = 2 x (1 - x^2);
  a[k_] := (1 + x^2)^2 - b Cos[Pi k/n];
  c0 = (1 - x)^m + x^m (1 + x)^m;
  s0 = (1 - x)^m - x^m (1 + x)^m;
  cn = (1 + x)^m + x^m (1 - x)^m;
  sn = (1 + x)^m - x^m (1 - x)^m;
  c2[k_] := (Sum[ m!/(2 j)!/(m - 2 j)! (a[k]^2 - b^2)^j a[k]^(m - 2 j), {j, 0, IntegerPart[m/2]}] + b^m)/2^(m - 1);
  s2[k_] := (Sum[ m!/(2 j)!/(m - 2 j)! (a[k]^2 - b^2)^j a[k]^(m - 2 j), {j, 0, IntegerPart[m/2]}] - b^m)/2^(m - 1);
  xp = Expand[ N[ (1/2) If[Mod[n, 2] == 0,
    Product[c2[2 k + 1], {k, 0, n/2 - 1}]
  + Product[s2[2 k + 1], {k, 0, n/2 - 1}]
  + c0 cn Product[c2[2 k], {k, 1, n/2 - 1}]
  + s0 sn Product[s2[2 k], {k, 1, n/2 - 1}],
    cn Product[c2[2 k + 1], {k, 0, (n - 3)/2}]
  + sn Product[s2[2 k + 1], {k, 0, (n - 3)/2}]
  + c0 Product[c2[2 k], {k, 1, (n - 1)/2}]
  + s0 Product[s2[2 k], {k, 1, (n - 1)/2}]], prec]];
  Take[Round[Chop[CoefficientList[xp, x]]], {1, -1, 2}]];

savels[fn_, ls_] := Module[{fp = OpenWrite[fn], i},
  For[i = 1, i <= Length[ls], i++, Write[fp, ls[[i]]]]; Close[fp]];

easydos[n_, m_] := Module[{dos = NDOS[n, m], logdos = Table[0, {n m + 1}], i},
  savels["IsingDOS" <> ToString[n] <> "x" <> ToString[m] <> ".txt", dos];
  For[i = 1, i <= n m + 1, i++,
    logdos[[i]] = If[dos[[i]] == 0, -10000, N[Log[dos[[i]]], 17]]];
  savels["islogdos" <> ToString[n] <> "x" <> ToString[m] <> ".txt", logdos]];
  */
INLINE int is2loadlogdos(double *logdos, int n, int m, const char *fn)
{
  char s[1024];
  FILE *fp;
  int i, err = 0;

  if (fn == NULL) { /* use standard file name */
    sprintf(s, "islogdos%dx%d.txt", n, m);
    fn = s;
  }
  xfopen(fp, fn, "r", return -1);

  for (i = 0; i <= n*m;  i++) {
    if (fgets(s, sizeof s, fp) == NULL) {
      printf("file %s ended at line %d/%d\n", fn, i, n*m);
      err = 1;
      break;
    }
    if (1 != sscanf(s, "%lf", &logdos[i])) {
      printf("sscanf failed in reading %s, line %d/%d\n", fn, i, n*m);
      err = 2;
      break;
    }
  }
  fclose(fp);
  return err;
}

#endif /* IS2_C__ */

