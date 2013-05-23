#include "cago.c"
#include "argopt.c"

const char *fnpdb = "pdb/1VII.pdb";
real kb = 200.f;
real ka = 40.f;
real kd1 = 1.f;
real kd3 = .5f;
real nbe = 1.f;
real nbc = 4.f; /* repulsion distance */
real rcc = 6.f;

real epot_target = 0;
int npass = 400;

static void doargs(int argc, char **argv)
{
  argopt_t *ao = argopt_open(0);
  ao->desc = "C-alpha GO model potential energy convergent";
  argopt_regarg(ao, NULL, &fnpdb, "pdbfile");
  argopt_reghelp(ao, "-h");
  argopt_regopt(ao, "-E", "%r", &epot_target, "target energy");
  argopt_regopt(ao, "-p","%d", &epot_target, "number of passes");
  argopt_parse(ao, argc, argv);
  argopt_close(ao);
}


/* guess a proper temperature for a target potential energy
 * return 0 if successful
 *
 * temperature is updated according to epot
 * several stages of updating are used, each with a fixed tpdt
 * after a stage, the updating magnitude amp is multiplied by ampf
 * iterations finish when the temperature difference is less than
 * a given tolerance 'tptol'
 * a pass is defined every time the potential energy crosses 'epot'
 * in every stage, npass passes are required to determine convergence
 * */
INLINE int cago_ucvgmdrun(cago_t *go, real mddt, real thermdt, int nstcom,
    real epot, int npass,
    real amp, real ampf, real tptol, av_t *avtp, av_t *avep, av_t *avrmsd,
    real tp, real tpmin, real tpmax, int tmax, int trep)
{
  int i, t, stg, sgp, sgn, ipass;
  real tpp = 0, tp1, tpav, epav, rdav, tmp;

  go->rmsd = cago_rmsd(go, go->x, NULL);
  sgp = (go->epot > epot) ? 1 : -1;
  for (stg = 0; ; stg++, amp *= ampf) { /* stages with different dpdt */
    if (avtp) av_clear(avtp);
    if (avep) av_clear(avep);
    if (avrmsd) av_clear(avrmsd);
    for (ipass = 0, t = 1; (tmax < 0 || t <= tmax) && ipass < npass; t++) {
      cago_vv(go, 1, mddt);
      if (t % nstcom == 0) cago_rmcom(go, go->x, go->v);
      cago_vrescale(go, (real) tp, thermdt);
      go->rmsd = cago_rmsd(go, go->x, NULL);
      sgn = (go->epot > epot) ? 1 : -1;
      if (sgn * sgp < 0) {
        ipass++;
        sgp = sgn;
      }
      /* update the temperature */
      tp1 = tp - sgn*mddt*amp;
      if (tp1 < tpmin) tp1 = tpmin;
      else if (tp1 > tpmax) tp1 = tpmax;
      for (tmp = tp1/tp, i = 0; i < go->n; i++) /* scale v */
        rv3_smul(go->v[i], tmp);
      tp = tp1;
      if (avtp) av_add(avtp, tp);
      if (avep) av_add(avep, go->epot);
      if (avrmsd) av_add(avrmsd, go->rmsd);
      if (trep >= 0 && t % trep == 0) {
        printf("%d|%9d: %.2f - %.2f, tp %.4f, K %.2f, rmsd %.4f pass: %d/%d\n",
            stg, t, go->epot, epot, tp,
            go->ekin, go->rmsd, ipass, npass);
      }
    }
    /* end of a stage */
    if (ipass < npass) { /* not enough passes over rmsd */
      const char fnfail[] = "fail.pos";
      go->rmsd = cago_rmsd(go, go->x, go->x1);
      cago_writepos(go, go->x1, NULL, fnfail);
      fprintf(stderr, "%d: failed to converge, epot: %g - %g, %d passes, %s\n",
          stg, epot, go->epot, ipass, fnfail);
      return 1;
    }
    tpav = av_getave(avtp);
    epav = av_getave(avep);
    rdav = av_getave(avrmsd);
    printf("%d: amp %g, tp %g, tpav %g/%g, epotav %g, rmsd %g, pass %d/%d\n",
        stg, amp, tp, tpav, tpp, epav, rdav, ipass, npass);
    tmp = .5*(tpav + tpp);
    if (stg > 0 && fabs(tpav - tpp) < tptol*tmp) break;
    tpp = tpav;
  }
  return 0;
}


int main(int argc, char **argv)
{
  cago_t *go;
  int ret;
  int nstcom = 10, tmax = 100000000, trep = 10000;
  real tptol = 0.01f, amp = 0.01f, ampf = sqrt(0.1);
  real mddt = 0.002f, thermdt = 0.02f;
  real tptry = 1.0, tpmin = 0.001, tpmax = 50.0;
  av_t avtp[1], avep[1], avrmsd[1];
  real tpav, tpdv, epav, epdv, rdav, rddv;

  doargs(argc, argv);
  if ((go = cago_open(fnpdb, kb, ka, kd1, kd3, nbe, nbc, rcc)) == NULL) {
    fprintf(stderr, "cannot initialize from %s\n", fnpdb);
    return 1;
  }
  cago_initmd(go, 0.1, 0.0);
  printf("%s n %d, epot = %g, %g, rmsd = %g\n", fnpdb, go->n, go->epot, go->epotref, go->rmsd);
  if (epot_target < go->epotref) {
    fprintf(stderr, "target energy %g must be greater than %g\n", epot_target, go->epotref);
    return 0;
  }

  ret = cago_ucvgmdrun(go, mddt, thermdt, nstcom,
      epot_target, npass, amp, ampf, tptol, avtp, avep, avrmsd,
      tptry, tpmin, tpmax, tmax, trep);
  tpav = av_getave(avtp);
  tpdv = av_getdev(avtp);
  epav = av_getave(avep);
  epdv = av_getdev(avep);
  rdav = av_getave(avrmsd);
  rddv = av_getdev(avrmsd);
  printf("ucvgmd %s, epot %7.4f, tp = %.4f(%.4f), epot = %.4f(%.4f), rmsd = %.2f(%.2f)\n",
      (ret ? "   failed" : "succeeded"), epot_target, tpav, tpdv, epav, epdv, rdav, rddv);
  cago_close(go);
  return 0;
}
