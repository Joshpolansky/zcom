/* Bennett acceptance ratio method applied to the matching of
   the square-well potential and the Lennard-Jones potential */
#define ZCOM_PICK
#define ZCOM_LJ
#define ZCOM_RPT
#define ZCOM_ARGOPT
#include "zcom.h"
#include "vmove.h"

int N = 108;
int D = 3;
real rho = 0.05f;
real ra = 1.0f; /* barrier distance of the square-well potential */
real rb = 1.5f; /* cutoff distance of the square-well potential */
int esqinf = 100000; /* infinity energy in square well potential */
real rc = 2.5f; /* cutoff distance for the Lennard-Jones potential */
real tp = 1.0f;
real amp = 0.3f; /* MC move size */
int nequil = 10000;
int nsteps = 100000;
int nevery = 10;  /* compute temperatures every this number of steps */
real ampp = 0.02f; /* smaller than MC size, to avoid large dU */
int dumax = 1000; /* histogram dimension */
real sqescl = 1.0;
char *fnehis = "ehsqmc.dat"; /* energy-increment distribution */
char *fnehislj = "ehljmc.dat"; /* from the Lennard-Jones potential */
int verbose = 0;

/* pressure code */
int isobaric; /* to be set by the program */
real pressure = 0.0f;
real lnvamplj = 0.07, lnvampsq = 0.02;

/* handle input arguments */
static void doargs(int argc, char **argv)
{
  argopt_t *ao = argopt_open(0);
  argopt_add(ao, "-n", "%d", &N,      "number of particles");
  argopt_add(ao, "-T", "%r", &tp,     "temperature");
  argopt_add(ao, "-r", "%r", &rho,    "density");
  argopt_add(ao, "-a", "%r", &ra,     "closest disance of the square-well potential");
  argopt_add(ao, "-b", "%r", &rb,     "the cutoff distance of the square-well potential");  
  argopt_add(ao, "-S", "%r", &sqescl, "energy unit of the square well potential");
  argopt_add(ao, "-c", "%r", &rc,     "the cutoff distance of the Lennard-Jones potential");  
  argopt_add(ao, "-0", "%d", &nequil, "number of equilibration");
  argopt_add(ao, "-1", "%d", &nsteps, "number of simulation steps");
  argopt_add(ao, "-e", "%d", &nevery, "interval of computing temperatures");  
  argopt_add(ao, "-m", "%r", &amp,    "amplitude of a MC move");
  argopt_add(ao, "-M", "%r", &ampp,   "amplitude of a perturbation");
  argopt_add(ao, "-U", "%d", &dumax,  "maximal energy change for histogram");
  argopt_add(ao, "-p", "%r", &pressure, "pressure");
  argopt_add(ao, "-o", NULL, &fnehis, "output file for the energy-increment histogram");
  argopt_add(ao, "-O", NULL, &fnehislj,"output file for that using the Lennard-Jones potential");
  argopt_add(ao, "-v", "%d", &verbose, "verbose mode");
  argopt_addhelp(ao, "-h");
  argopt_parse(ao, argc, argv);

  if (rb <= ra) {
    fprintf(stderr, "rb %g is less than ra %g, simulating hard balls\n", rb, ra);
    rb = ra;
  }
  if (pressure > 1e-6) isobaric = 1;
  if (isobaric) rc = 1000;

  if (verbose >= 0) argopt_dump(ao);
  argopt_close(ao);
}

/* compute dlnZ from Bennett's acceptance ratio method
 * c0 are c1 are the actual # of data points
 * while histograms do not collect infinities */ 
static double bardlnZ(hist_t *hs0, double c0, hist_t *hs1, double c1,
    double *dS0, double *dS1, double *frac0, double *frac1)
{
  double C0 = 0, C1 = 0, C1n, *h0 = hs0->arr, *h1 = hs1->arr, x, s0, s1, sf0, sf1;
  int iter, i, i0s, i0t, i1s, i1t, n = hs0->n, hasinf = 0;

  /* search for the boundaries */
  for (i0s = 0; i0s < n; i0s++) if (h0[i0s] > 0) break;
  for (i0t = n; i0t > i0s; i0t--) if (h0[i0t - 1] > 0) break;
  for (*dS0 = 0, s0 = 0, i = i0s; i < i0t; i++) {
    if (h0[i] < 0) continue;
    x = hs0->xmin + hs0->dx * (i + .5);
    s0 += h0[i];
    *dS0 += h0[i] * x;
  }
  if (s0 > 0) *dS0 /= s0;
  *frac0 = 1 - s0/c0;
  if (s0 < c0 - .5) hasinf += 1;

  for (i1s = 0; i1s < n; i1s++) if (h1[i1s] > 0) break;
  for (i1t = n; i1t > i1s; i1t--) if (h1[i1t - 1] > 0) break;
  for (*dS1 = 0, s1 = 0, i = i1s; i < i1t; i++) {
    if (h1[i] < 0) continue;
    x = hs1->xmin + hs1->dx * (i + .5);
    s1 += h1[i];
    *dS1 += h1[i] * x; 
  }
  if (s1 > 0) *dS1 /= s1;
  *frac1 = 1 - s1/s0;
  if (s1 < c1 - .5) hasinf += 2;

  if (s0 <= 0 && s1 <= 0) { /* no data */
    fprintf(stderr, "s0 %g, s1 %g\n", s0, s1);
    return 0;
  }
 
  /* get a rough estimate, by Z0/Z1 = < exp(U1 - U0) >_1 */
  if (s1 > 0) {
    for (sf1 = -1e100, i = i1s; i < i1t; i++) {
      if (h1[i] <= 0) continue;
      x = hs1->xmin + hs1->dx * (i + .5);
      sf1 = lnadd(sf1, -x + log(h1[i]));
    }
    sf1 -= log(c1);
    C1 = sf1;
  }
  
  /* get another rough estimate, by Z1/Z0 = < exp(U0 - U1) >_0 */
  if (s0 > 0) {
    for (sf0 = -1e100, i = i0s; i < i0t; i++) {
      if (h0[i] <= 0) continue;
      x = hs0->xmin + hs0->dx * (i + .5);
      sf0 = lnadd(sf0, -x + log(h0[i]));
    }
    sf0 -= log(c0);
    C0 = -sf0;
  }
  fprintf(stderr, "initial guess, s0 %g(%g), s1 %g(%g), dlnZ %g(from LJ), %g(from sq) dSLJ %g%s, dSsq %g%s\n",
      s0, c0, s1, c1, C0, C1,
      (hasinf&1) ? *frac0 : *dS0 - C0, (hasinf&1) ? "inf" : "",
      (hasinf&2) ? *frac1 : *dS1 + C1, (hasinf&2) ? "inf" : "");

  if (s0 <= 0) { /* use hs1 only */
    *dS0 -= C1;
    *dS1 += C1;
    return C1;
  } else if (s1 <= 0) { /* use hs0 only */
    *dS0 -= C0;
    *dS1 += C0;
    return C0;
  }

  for (iter = 0; iter < 1000; iter++) {
    /* compute <f(U0 - U1 + C1)>_1 */
    for (sf1 = -1e100, i = i1s; i < i1t; i++) {
      if (h1[i] <= 0) continue;
      x = hs1->xmin + hs1->dx * (i + .5) + C1;
      x = -lnadd(x, 0); /* 1/(1 + exp(x)) */
      sf1 = lnadd(sf1, x + log(h1[i]));
    }
    sf1 -= log(c1);

    /* compute <f(U1 - U0 - C1)>_0 */
    for (sf0 = -1e100, i = i0s; i < i0t; i++) {
      if (h0[i] <= 0) continue;
      x = hs0->xmin + hs0->dx * (i + .5) - C1;
      x = -lnadd(x, 0); /* 1/(1 + exp(x)) */
      sf0 = lnadd(sf0, x + log(h0[i]));
    }
    sf0 -= log(c0);
    
    C1n = C1 + sf1 - sf0 + log(c1/c0);
    if (verbose > 0) {
      fprintf(stderr, "sf1 %g, sf0 %g, dlnZ %g, dlnZ' %g\n", sf1, sf0, C1, C1n);
      if (verbose >= 2) getchar();
    }
    if (fabs(C1 - C1n) < 1e-6) break;
    C1 = (C1 + C1n)*.5; /* to prevent oscillation, but slows down convergence */
  }

  C1 -= log(c1/c0); /* dlnZ */
  *dS0 -= C1;
  *dS1 += C1;
  return C1;
}

/* do Monte Carlo simulation and compute the perturbation temperature */
static void domc(lj_t *lj, lj_t *sq)
{
  int t, acclj = 0, accsq = 0;
  int id, idu;
  double epslj, vir, bpi, bplji;
  double Ulj, Usq, UljB, UsqB, Uljref, plj, pljB;
  double dlnZ = 0, dSsq = 0, dSlj = 0, hscnt = 0, frlj = 0, frsq = 0;
  static av_t avUsq[1], avUlj[1], avUsqB[1], avUljB[1], avdesq[1], avdelj[1];
  static av_t avplj[1], avpljB[1], avrholj[1], avrhosq[1];
  rpti_t *rptsq;
  rpt_t *rptlj, *rptv;
  hist_t *hlj, *hsq; /* energy histograms */
  double vtot = 1e-30, vaccsq = 0, vacclj = 0;

  rptsq = rpti_open(-dumax, dumax, 1, RPTI_HASINF);
  rptlj = rpt_open(-dumax, dumax, 0.0002);
  rptv = rpt_open(-lj->n, lj->n, 0.001f);
  hlj = hs_open(1, -20*lj->n, 20*lj->n, 0.001*lj->n);
  hsq = hs_open(1, -20*lj->n, 20*lj->n, 0.001*lj->n);
  
  for (t = 0; t < nequil; t++) { /* warm up */
    lj_metro3d(lj, amp, 1.0/tp);
    lj_metro3d(sq, amp, (sqescl/tp));  /* exp(-sqescl * de / tp) */
  }

  for (t = 1; t <= nsteps; t++) { /* real simulation */
    acclj += lj_metro3d(lj, amp, 1.0/tp);
    accsq += lj_metro3d(sq, amp, (sqescl/tp));
    
    av_add(avUlj, lj->epot);
    plj = lj_calcp(lj, tp);
    av_add(avplj, plj);
    av_add(avUsq, sq->epot * sqescl);

    /* try local perturbations often, for they are cheap */
    {
      real xi[3];

      /* in the LJ system, compute the increment of the square-well energy */
      id = lj_randmv3d(lj, xi, ampp);
      idu = lj_depotsq3d(lj, id, xi, NULL);
      if (idu > -esqinf/2 && idu < esqinf/2) { /* idu may be negative infinity for the inverse mode */
        av_add(avdesq, idu * sqescl);
        rpti_add(rptsq, idu);
      }
      
      /* in the square-well system, compute the increment of the LJ energy */ 
      id = lj_randmv3d(sq, xi, ampp);
      epslj = lj_depotlj3d(sq, id, xi, &vir);
      rpt_add(rptlj, epslj);
      av_add(avdelj, epslj);
    }

    if (t % nevery == 0) {
      hscnt++;

      /* use fixed virtual move */
      sq_vmove(sq, -0.01, tp/sqescl, rptv, 1);

      /* try to change the volume */
      if (isobaric) {
        vtot++;
        vacclj += lj_mcplj(lj, lnvamplj, tp, pressure, 0, 1e30, 0, 0);
        av_add(avrholj, lj->n / lj->vol);
        /* the coupling for the volume is pressure/tp,
         * which is unaffected by sqescl */
        vaccsq += lj_mcpsq(sq, lnvampsq, tp/sqescl, pressure/sqescl, 0, 1e30, 0, 0);
        av_add(avrhosq, sq->n / sq->vol);
      }

      /* compute <Usq> in the LJ system */
      Ulj = lj->epot;
      Usq = lj_energysq3d(lj, (rv3_t *) lj->x, &lj->rmin);
      if (sqescl > 0) {
        Usq *= sqescl;
      } else if (Usq <= 0) { /* escl == 0 */
        Usq *= sqescl;
      }
      if (Usq <= 0) { /* collect configurations with no clash */
        av_add(avUsqB, Usq);
        hs_add1ez(hlj, (Usq - Ulj)/tp, HIST_VERBOSE);
      }
      
      /* compute <Ulj> in the sq system */ 
      Usq = sqescl * sq->epot;
      Ulj = lj_energylj3d(sq, (rv3_t *) sq->x, &sq->vir, NULL, NULL);
      av_add(avUljB, Ulj);
      hs_add1ez(hsq, (Ulj - Usq)/tp, HIST_VERBOSE);
      pljB = lj_calcp(sq, tp);
      av_add(avpljB, pljB);
    }
  }
  Usq = av_getave(avUsq)/lj->n;
  Ulj = av_getave(avUlj)/lj->n;
  UsqB = av_getave(avUsqB)/lj->n;
  UljB = av_getave(avUljB)/lj->n;
  plj  = av_getave(avplj);
  pljB = av_getave(avpljB);
  if (isobaric)
    rho = av_getave(avrholj);
  Uljref = lj_eos3d(rho, tp, NULL, NULL, NULL);

  bpi = (fabs(ra - rb) > 1e-6 ? rpti_bet(rptsq, 0)/sqescl : 0); /* we got s * bet, so divide s */
  bplji = rpt_bet(rptlj, 0);

  /* compute the relative entropy */
  if (avUsqB->s <= 0.) { /* we have no useful data */
    fprintf(stderr, "Warning: LJ system always produces clashed square-well conf.\n");
    hs_clear(hlj); /* force to use the hsq only */
  }
  dlnZ = bardlnZ(hlj, hscnt, hsq, hscnt, &dSlj, &dSsq, &frlj, &frsq);
  fprintf(stderr, "T %g, r %g, a %g, b %g, es %g, amp %g, pamp %g, "
         "accsq %.3f, acclj %.3f, desq %.4f(%.4f), delj %.4f(%.4f), "
         "Usqlj %.4f, Usq %.4f, bpi %.4f, psq %.4f, "
         "Uljsq %.4f, Ulj %.4f, Uljref %.4f, pljB %.4f, plj %.4f, "
         "bplj0 %.4f, %.4f (%g), "
         "vacc %.2f(lj), %.2f(sq), rho %.4f(lj), %.4f(sq), "
         "dSlj %g%s, dSsq %g%s, dlnZ %g\n",
         tp, rho, ra, rb, sqescl, amp, ampp,
         1.*accsq/nsteps, 1.*acclj/nsteps,
         av_getave(avdesq), av_getdev(avdesq),
         av_getave(avdelj), av_getdev(avdelj),
         UsqB, Usq, bpi, rpt_betw(rptv) * tp, 
         UljB, Ulj, Uljref, pljB, plj,
         bplji, rpt_bets(rptlj, 1), 1.0/tp,
         vacclj/vtot, vaccsq/vtot, av_getave(avrholj), av_getave(avrhosq),
         (avUsqB->s < hscnt-.5) ? frlj : dSlj, (avUsqB->s < hscnt-.5) ? "inf" : "",
         (avUljB->s < hscnt-.5) ? frsq : dSsq, (avUljB->s < hscnt-.5) ? "inf" : "", dlnZ);
     
  rpt_wdist(rptlj, fnehislj);
  rpti_wdist(rptsq, fnehis);
  rpt_close(rptlj);
  rpti_close(rptsq);
  rpt_close(rptv);
  hs_close(hlj);
  hs_close(hsq); 
}

int main(int argc, char **argv)
{
  lj_t *lj, *sq;

  doargs(argc, argv);

  lj = lj_open(N, D, rho, rc);
  sq = lj_open(N, D, rho, rc);

  lj_initsq(lj, ra, rb); /* we still need to pass ra & rb */
  lj->usesq = 0;
  lj_energy(lj);
  
  lj_initsq(sq, ra, rb);
  sq->esqinf = lj->esqinf = esqinf;
  
  domc(lj, sq);

  lj_close(lj);
  lj_close(sq);
  return 0;
}
