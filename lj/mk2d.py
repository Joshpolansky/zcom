#!/usr/bin/env python
""" create 2d versions from 3d versions """
import os, add2d

if __name__ == "__main__":
  output = "lj.c"
  s = os.path.splitext(output)
  input = s[0] + ".0" + s[1]
  add2d.add2d(input, output, [
    "lj_pbcdist2_3d",
    "lj_randmv3d", 
    "lj_pairsq3d", "lj_depotsq3d", "lj_commitsq3d", "lj_metrosq3d", 
    "lj_pair3d", "lj_depot3d", "lj_commit3d", "lj_metro3d", 
    "lj_energysw3d", "lj_energysq3d", "lj_energylj3d", "lj_energy3d",
    "lj_forcesw3d", "lj_forcelj3d", "lj_force3d"])