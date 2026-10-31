#include <omp.h>
#include <cmath>

#include "data_type.h"

void do_n_CPU(int nx, int ny, int nz, real dx_1, real dy_1, real dz_1, real dt,
                  const real* jx,  const real* jy,  const real* jz,
                  real* n)
{  
  #pragma omp parallel for
  for(int k=0;k<nz;k++) {
    for(int j=0;j<ny;j++) {
      for(int i=0;i<nx;i++) {
        if(n[i + j*nx + k*ny*nx] < (real) .0)
          continue;
        n[i + j*nx + k*ny*nx] -= ((jx[(i+1) + j*(nx+1) + k*ny*(nx+1)] - jx[i + j*(nx+1) + k*ny*(nx+1)]) * dx_1 + 
                                  (jy[i + (j+1)*nx + k*(ny+1)*nx] - jy[i + j*nx + k*(ny+1)*nx]) * dy_1 + 
                                  (jz[i + j*nx + (k+1)*ny*nx] - jz[i + j*nx + k*ny*nx]) * dz_1) * dt;
        if(n[i + j*nx + k*ny*nx] < (real) .0)
          n[i + j*nx + k*ny*nx] = (real) .0;
      }
    }
  }
}

void do_dj_CPU(int nx, int ny, int nz, real dx_1, real dy_1, real dz_1,
               const real* _n, const real* _jx, const real* _jy, const real* _jz,
               real* _djx, real* _djy, real* _djz,
               real T0, real R)
{
  const real gamma = 1.; // (real) 5. / (real) 3.;
  #pragma omp parallel for
  for(int _k=0;_k<nz+1;_k++) {
    for(int _j=0;_j<ny+1;_j++) {
      for(int _i=0;_i<nx+1;_i++) {

        #define DOJ  \
          real n_px = n(i,j,k); \
          real n_c = n(i-1,j,k); \
          real jx = jx(i,j,k); \
          real vx = (n_c < (real).0 || n_px < (real).0) ? (real).0 : (real) 2. * jx / (n_c + n_px); \
          real jx_px = jx(i+1,j,k); \
          real vx_p_2x = (real) .5 * (jx_px + jx) / n_px; \
          real jx_mx = jx(i-1,j,k); \
          real vx_m_2x = (real) .5 * (jx_mx + jx) / n_c; \
           \
          real n_py = n(i-1, j+1, k); \
          real n_px_py = n(i, j+1, k); \
          real jx_py = jx(i, j+1, k); \
          real vx_p_2y = (real) .5 * (vx + ((n_py < (real).0 || n_px_py < (real).0) ? (real).0 : (real) 2. * jx_py / (n_py + n_px_py) )); \
          real n_my = n(i-1, j-1, k); \
          real n_px_my = n(i, j-1, k); \
          real jx_my = jx(i, j-1, k); \
          real vx_m_2y = (real) .5 * (vx + ((n_my < (real).0 || n_px_my < (real).0) ? (real).0 : (real) 2. * jx_my / (n_my + n_px_my) )); \
           \
          real jy_my = (real) .5 * (jy(i, j, k) + jy(i-1, j, k)); \
          real jy_py = (real) .5 * (jy(i, j+1 ,k) + jy(i-1, j+1, k)); \
           \
          real n_pz = n(i-1, j, k+1); \
          real n_px_pz = n(i, j, k+1); \
          real jx_pz = jx(i, j, k+1); \
          real vx_p_2z = (real) .5 * (vx + ((n_pz < (real).0 || n_px_pz < (real).0) ? (real).0 : (real) 2. * jx_pz / (n_pz + n_px_pz) )); \
          real n_mz = n(i-1, j, k-1); \
          real n_px_mz = n(i, j, k-1); \
          real jx_mz = jx(i, j, k-1); \
          real vx_m_2z = (real) .5 * (vx + ((n_mz < (real).0 || n_px_mz < (real).0) ? (real).0 : (real) 2. * jx_mz / (n_mz + n_px_mz) )); \
           \
          real jz_mz = (real) .5 * (jz(i, j, k) + jz(i-1, j, k)); \
          real jz_pz = (real) .5 * (jz(i, j, k+1) + jz(i-1, j, k+1)); \
           \
          djx(i, j, k) =  R*( (jx_px + jx_mx - (real) 2. * jx) * dx_1 * dx_1 \
                             +(jx_py + jx_my - (real) 2. * jx) * dy_1 * dy_1 \
                             +(jx_pz + jx_mz - (real) 2. * jx) * dz_1 * dz_1) \
                          - (real) .5 * (jx_px * vx_p_2x - jx_mx * vx_m_2x) * dx_1  \
                          - (jy_py * vx_p_2y - jy_my * vx_m_2y) * dy_1  \
                          - (jz_pz * vx_p_2z - jz_mz * vx_m_2z) * dz_1  \
                          - T0 * (pow(n_px, gamma) - pow(n_c, gamma)) * dx_1 \
                          + (dir==0 ? (real).0 : 0); \
          if(n_c < (real) .0 || n_px < (real) .0) \
            djx(i, j, k) = (real) .0;


                          /*
                          - (jx_px * vx_p_2x - jx_mx * vx_m_2x) * dx_1  \
                          - (jy_py * vx_p_2y - jy_my * vx_m_2y) * dy_1  \
                          - (jz_pz * vx_p_2z - jz_mz * vx_m_2z) * dz_1  \
                          */

/*
                          - jx * (vx_p_2x - vx_m_2x) * dx_1  \
                          - (real) .5 * (jy_py + jy_my) * (vx_p_2y - vx_m_2y) * dy_1  \
                          - (real) .5 * (jz_pz + jz_mz) * (vx_p_2z - vx_m_2z) * dz_1  \
*/

/*
        #define DOJ  \
          real n_px = n(i,j,k); \
          real n_c = n(i-1,j,k); \
          djx(i, j, k) =  - T0 * (n_px - n_c) * dx_1; \
          if(n_c < (real) .0 || n_px < (real) .0) \
            djx(i, j, k) = (real) .0;
*/
        
        int dir=0;
        //X
        if(_i > 0 && _i < nx && _j < ny-1 && _j > 0 && _k < nz-1 && _k > 0)
        {
          const int i = _i;
          const int j = _j;
          const int k = _k;
          #define n(I,J,K) _n[I+(J)*nx+(K)*nx*ny]
          #define jx(I,J,K) _jx[I + (J)*(nx+1) + (K)*ny*(nx+1)]
          #define jy(I,J,K) _jy[I + (J)*nx + (K)*(ny+1)*nx]
          #define jz(I,J,K) _jz[I + (J)*nx + (K)*ny*nx]
          #define djx(I,J,K) _djx[I + (J)*(nx+1) + (K)*ny*(nx+1)]
          #define djy(I,J,K) _djy[I + (J)*nx + (K)*(ny+1)*nx]
          #define djz(I,J,K) _djz[I + (J)*nx + (K)*ny*nx]
          DOJ
          #undef djz
          #undef djy
          #undef djx
          #undef jz
          #undef jy
          #undef jx
          #undef n
        }
        
        dir=1;
        //Y
        if(_i > 0 && _i < nx-1 && _j < ny && _j > 0 && _k < nz-1 && _k > 0)
        {
          const int i = _j;
          const int j = _i;
          const int k = _k;
          #define n(I,J,K) _n[J+(I)*nx+(K)*nx*ny]
          #define jx(I,J,K) _jy[J + (I)*nx + (K)*(ny+1)*nx]
          #define jy(I,J,K) _jx[J + (I)*(nx+1) + (K)*ny*(nx+1)]
          #define jz(I,J,K) _jz[J + (I)*nx + (K)*ny*nx]
          #define djx(I,J,K) _djy[J + (I)*nx + (K)*(ny+1)*nx]
          #define djy(I,J,K) _djx[J + (I)*(nx+1) + (K)*ny*(nx+1)]
          #define djz(I,J,K) _djz[J + (I)*nx + (K)*ny*nx]
          DOJ
          #undef djz
          #undef djy
          #undef djx
          #undef jz
          #undef jy
          #undef jx
          #undef n
        }
        
        /*
        dir=2;
        //Z
        if(_i > 0 && _i < nx-1 && _j < ny-1 && _j > 0 && _k < nz && _k > 0)
        {
          const int i = _k;
          const int j = _j;
          const int k = _i;
          #define n(I,J,K) _n[K+(J)*nx+(I)*nx*ny]
          #define jx(I,J,K) _jz[K + (J)*nx + (I)*ny*nx]
          #define jy(I,J,K) _jy[K + (J)*nx + (I)*(ny+1)*nx]
          #define jz(I,J,K) _jx[K + (J)*(nx+1) + (I)*ny*(nx+1)]
          #define djx(I,J,K) _djz[K + (J)*nx + (I)*ny*nx]
          #define djy(I,J,K) _djy[K + (J)*nx + (I)*(ny+1)*nx]
          #define djz(I,J,K) _djx[K + (J)*(nx+1) + (I)*ny*(nx+1)]
          DOJ
          #undef djz
          #undef djy
          #undef djx
          #undef jz
          #undef jy
          #undef jx
          #undef n
        }
*/
      }
    }
  }
}

void do_j_CPU(int nx, int ny, int nz, real dt,
                  real* jx, real* jy, real* jz,
                  const real* djx, const real* djy, const real* djz) {

  //BLOCK : 16*16*16
  //WF : >=16
  //WG : 256 ... or whatever
  
  #pragma omp parallel for
  for(int k=0;k<nz+1;k++) {
    for(int j=0;j<ny+1;j++) {
      for(int i=0;i<nx+1;i++) {
        if(i >= nx+1 || j >= ny+1 || k >= nz+1)
          continue;
        //now do the job!
        if(i < nx+1 && j < ny && k < nz)
          jx[i+j*(nx+1)+k*(nx+1)*ny] += djx[i+j*(nx+1)+k*(nx+1)*ny] * dt;
        if(i < nx && j < ny+1 && k < nz)
          jy[i+j*nx+k*nx*(ny+1)] += djy[i+j*nx+k*nx*(ny+1)] * dt;
        if(i < nx && j < ny && k < nz+1)
          jz[i+j*nx+k*nx*ny] += djz[i+j*nx+k*nx*ny] * dt;
      }
    }
  }
}
