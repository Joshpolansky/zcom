#include "rv3.h"
#include "rv2.h"
#include "md.h"
#ifndef ABPRO_H__
#define ABPRO_H__

typedef struct {
  int d; /* dimension */
  int model; /* 1 or 2 */
  int seqid; /* model sequence id */
  int n; /* number of atoms */
  int dof; /* number of degrees of freedom */
  real clj[2][2], sla, slb;
  int *type; /* 0: A, 1: B */
  real *x, *x1, *dx;
  real *v;
  real *f;
  real *lmx, *xmin;
  real emin, epot, ekin, tkin;
  double t;
} abpro_t;

#define AB_VERBOSE    0x0001
#define AB_SOFTFORCE  0x0010
#define AB_MILCSHAKE  0x0020
#define AB_LMREGISTER 0x0100
#define AB_LMWRITE    0x0200

abpro_t *ab_open(int seqid, int d, int model, real randdev);
void ab_close(abpro_t *ab);

int ab_checkconn(abpro_t *ab, const real *x, double tol);
#define ab_shiftcom(ab, x)      md_shiftcom(x, ab->d, ab->n)
#define ab_shiftang(ab, x, v)   md_shiftang(x, v, ab->d, ab->n)
void ab_rmcom(abpro_t *ab, real *x, real *v);

int ab_writepos(abpro_t *ab, const real *x, const real *v, const char *fname);
int ab_readpos(abpro_t *ab, real *x, real *v, const char *fname);
int ab_initpos(abpro_t *ab, real *x, real randev);

int ab_shake(abpro_t *ab, const real *x0, real *x1, real *v, real dt, 
    int itmax, double tol, int verbose);
int ab_rattle(abpro_t *ab, const real *x0, real *v, 
    int itmax, double tol, int verbose);
int ab_milcshake(abpro_t *ab, const real *x0, real *x1, real *v, real dt,
    int itmax, double tol, int verbose);
int ab_milcrattle(abpro_t *ab, const real *x0, real *v); 

real ab_localmin(abpro_t *ab, const real *r, int itmax, double tol,
    int sh_itmax, double sh_tol, unsigned flags);
real ab_energy(abpro_t *ab, const real *r, int soft);
real ab_force(abpro_t *ab, real *f, const real *r, int soft);

ZCINLINE real ab_ekin(abpro_t *ab)
  { return ab->ekin = md_ekin(ab->v, ab->n*ab->d, ab->dof, &ab->tkin); }
ZCINLINE void ab_vrescale(abpro_t *ab, real tp, real dt)
  { md_vrescale(tp, dt, &ab->ekin, &ab->tkin, ab->v, ab->n*ab->d, ab->dof); }
int ab_vv(abpro_t *ab, real fscal, real dt, unsigned flags);
int ab_brownian(abpro_t *ab, real T, real fscal, real dt, unsigned flags);

#endif
