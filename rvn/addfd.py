#!/usr/bin/env python
''' add float and double versions of the functions before the real-version functions '''


import re, os, sys

sys.path.insert(0, "..")
from python import ccom

ccom.addfd(None, "rvn.h", [
  "rvn_fprint", "rvn_print",
  "rvn_make",
  "rvn_zero",
  "rvn_copy", "rvn_ncopy", "rvn_swap",
  "rvn_sqr", "rvn_norm", "rvn_dot",
  "rvn_neg", "rvn_neg2",
  "rvn_inc", "rvn_dec", "rvn_sinc", "rvn_smul", "rvn_smul2",
  "rvn_normalize", "rvn_makenorm",
  "rvn_diff", "rvn_dist2", "rvn_dist", "rvn_add", "rvn_nadd",
  "rvn_sadd", "rvn_lincomb2", "rvn_fma",
  "rvn_cosang", "rvn_ang",
  "rvn_rnd0", "rvn_rnd", "rvn_grand0", "rvn_grand",
  "rvn_rand01", "rvn_randunif", "rvn_randgaus", "rvn_randgausdisp",
  "rvn_randdir0", "rvn_randdir", "rvn_randball0", "rvn_randball",
  ])

ccom.addfd(None, "rmn.h", [
  "rmn_fprint", "rmn_print",
  "rmn_make",
  "rmn_zero", "rmn_one",
  "rmn_randuni",
  ])

