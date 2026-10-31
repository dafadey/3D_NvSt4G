#pragma once

#include <omp.h>

void do_n_CPU(int nx, int ny, int nz, real dx_1, real dy_1, real dz_1, real dt,
                  const real* jx,  const real* jy,  const real* jz,
                  real* n);
void do_dj_CPU(int nx, int ny, int nz, real dx_1, real dy_1, real dz_1,
               const real* _n, const real* _jx, const real* _jy, const real* _jz,
               real* _djx, real* _djy, real* _djz,
               real T0, real R);

void do_j_CPU(int nx, int ny, int nz, real dt,
                  real* jx, real* jy, real* jz,
                  const real* djx, const real* djy, const real* djz);
