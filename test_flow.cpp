#include "data_type.h"
#include "nvst.h"
#include "nvst_CPU.h"
#include "timer.h"
#include "simpledraw2D.h"

#include <fstream>
#include <cmath>
#include <iostream>
#include <array>
#include <fstream>


#define SQR(X) (X*X)

int main(int argc, char** argv) {
  fadey_init(13,13,4);
  
  real T0 = 3.3;
  real R = 0.0003;
  real n0 = 1.;
  real jmax = 1.1;
    
  int step_count = 13;
  
  typedef std::vector<std::array<real,3>> trajectory_type;
  
  if(argc > 1)
    step_count = atoi(argv[1]);
  
  nvst h;
  h.nx=303; //63 is ok for J1900
  h.ny=303;
  h.nz=303;
  h.np=3333;
  h.Lx = 1.;
  h.Ly = 1.;
  h.Lz = 1.;
  h.T0 = T0;
  h.R = R;

  std::vector<trajectory_type> trajectories(h.np);

  if(!h.init())
    return -1;

//  real dt = 0.0003;
  real dt = std::min(.05/h.dx_1, .25/(h.R*h.dx_1*h.dx_1));
  
  char a;
  //std::cout << "enter to proocced to setting initial data\n";
  //std::cin >> a;

  real vortex_r = 0.05;//0.042;
  real vortex_pos[] = {.0, -.1};


  for(int k=0;k<h.nz;k++) {
    for(int j=0;j<h.ny;j++) {
      for(int i=0;i<h.nx;i++) {
        real x = real(i - h.nx/2) / h.dx_1;
        real y = real(j - h.ny/2) / h.dy_1;
        real z = real(k - h.nz/2) / h.dz_1;
        size_t addr = i+j*h.nx+k*h.nx*h.ny;
        h.host_n[addr] = n0*(.25*tanh(-x/0.1)+0.8);
        //h.host_jx[i+j*(h.nx+1)+k*(h.nx+1)*h.ny] = exp(-SQR(SQR(x/.13))) * exp(-SQR(y/.17)) * exp(-SQR(z/.17)) * jmax;
        ////h.host_jy[i+j*h.nx+k*h.nx*(h.ny+1)] = exp(-SQR(x/0.03)) * exp(-SQR(y/0.03)) * exp(-SQR(z/0.03)) * jmax;

        real xv = x-vortex_pos[0];
        real yv = y-vortex_pos[1];
        
        h.host_jx[i+j*(h.nx+1)+k*(h.nx+1)*h.ny] += 22. * yv * exp(-SQR(xv/vortex_r)) * exp(-SQR(yv/vortex_r)) * exp(-SQR(z/.22)) * jmax;
        h.host_jy[i+j*h.nx+k*h.nx*(h.ny+1)] += -22. * xv * exp(-SQR(xv/vortex_r)) * exp(-SQR(yv/vortex_r)) * exp(-SQR(z/.22)) * jmax;

      }
    }
  }

  for(int k=0;k<h.nz;k++) {
    for(int j=0;j<h.ny;j++) {
      for(int i=0;i<h.nx;i++) {
        real x = (real) i / h.dx_1;
        real y = (real) j / h.dy_1;
        real z = (real) k / h.dz_1;
        size_t addr = i+j*h.nx+k*h.nx*h.ny;
        if(std::abs(x-h.Lx*.63) < h.Lx*.01 && std::abs(y-h.Ly*.69) < h.Ly*.17 && std::abs(z-h.Lz*.55) < h.Lz*.22)
          h.host_n[addr] = -.1;
      }
    }
  }

  std::cout << "generating particles..." << std::flush;
  for(int ip=0;ip<h.np;ip++) {
    int i = (int) (.55 * h.dx_1);
    int j = rand() % h.ny;
    int k = rand() % h.nz;
    h.host_particles[ip] = (real) i / h.dx_1;
    h.host_particles[ip + h.np] = (real) j / h.dy_1;
    h.host_particles[ip + 2 * h.np] = (real) k / h.dz_1;

/*
    while(true) {
      int i = rand() % h.nx;
      int j = rand() % h.ny;
      int k = rand() % h.nz;
      real jabs = std::sqrt(SQR(h.host_jx[i+j*(h.nx+1)+k*(h.nx+1)*h.ny]) + SQR(h.host_jy[i+j*h.nx+k*h.nx*(h.ny+1)])+ SQR(h.host_jz[i+j*h.nx+k*h.nx*h.ny]));
      if(rand() % 1024 < jabs/jmax*1024) {
        h.host_particles[ip] = (real) i / h.dx_1;
        h.host_particles[ip + h.np] = (real) j / h.dy_1;
        h.host_particles[ip + 2 * h.np] = (real) k / h.dz_1;
        break;
      }
    }
*/
  }
  std::cout << "done\n";

  h.set_data();
  
  std::cout << "for now theoretical memory consumption is " << ((h.nx*h.ny*h.nz*sizeof(real)*7) >> 20) << " Mb\n";
  
  //std::cout << "enter to proocced to kernel run\n";
  //std::cin >> a;
  
  std::vector<real> n_2D_xy_slice(h.nx*h.ny);
  std::vector<real> jx_2D_xy_slice((h.nx+1)*h.ny);
  std::vector<real> jy_2D_xy_slice(h.nx*(h.ny+1));
  std::vector<real> jz_2D_xy_slice(h.nx*h.ny);
  
  timer t("");
  std::cout << "running CPU kernel\n";
  t.reset();

  for(int c=0;c<1+0*step_count;c++) {
    std::cout << c << ' ' << std::flush;
    do_n_CPU(h.nx, h.ny, h.nz, h.dx_1, h.dy_1, h.dz_1, dt,
             h.host_jx.data(),  h.host_jy.data(),  h.host_jz.data(),
             h.host_n.data());
    do_dj_CPU(h.nx, h.ny, h.nz, h.dx_1, h.dy_1, h.dz_1,
              h.host_n.data(), h.host_jx.data(), h.host_jy.data(), h.host_jz.data(),
              h.host_djx.data(), h.host_djy.data(), h.host_djz.data(), T0, R);
    do_j_CPU(h.nx, h.ny, h.nz, dt,
             h.host_jx.data(), h.host_jy.data(), h.host_jz.data(),
             h.host_djx.data(), h.host_djy.data(), h.host_djz.data());
    for(int j=0;j<h.ny;j++){
      for(int i=0;i<h.nx;i++)
        n_2D_xy_slice[i+j*h.nx] = h.host_n[i+j*h.nx+(h.nz>>1)*h.nx*h.ny];
    }
    for(int j=0;j<h.ny;j++){
      for(int i=0;i<h.nx+1;i++)
        jx_2D_xy_slice[i+j*(h.nx+1)] = h.host_jx[i+j*(h.nx+1)+(h.nz>>1)*(h.nx+1)*h.ny];
    }
    for(int j=0;j<h.ny+1;j++){
      for(int i=0;i<h.nx;i++)
        jy_2D_xy_slice[i+j*h.nx] = h.host_jy[i+j*h.nx+(h.nz>>1)*h.nx*(h.ny+1)];
    }
    for(int j=0;j<h.ny;j++){
      for(int i=0;i<h.nx;i++)
        jz_2D_xy_slice[i+j*h.nx] = h.host_jz[i+j*h.nx+(h.nz>>1)*h.nx*h.ny];
    }
    fadey_draw(n_2D_xy_slice.data(), h.nx, h.ny, 0);
    fadey_draw(jx_2D_xy_slice.data(), h.nx+1, h.ny, 1);
    fadey_draw(jy_2D_xy_slice.data(), h.nx, h.ny+1, 2);
    fadey_draw(jz_2D_xy_slice.data(), h.nx, h.ny, 3);
    //char a;
    //std::cin >> a;
  
  }

  std::cout << '\n';
  std::cout << "CPU time is " << t.elapsed() << " s\n";

  for(int i=0;i<h.nx;i++)
    std::cout << h.host_n[i+(h.ny/2)*h.nx+h.nx*h.ny*(h.nz/2)] << ' ';
  std::cout << '\n';

  for(int i=0;i<h.nx+1;i++)
    std::cout << h.host_jx[i+(h.ny/2)*(h.nx+1)+(h.nx+1)*h.ny*(h.nz/2)] << ' ';
  std::cout << '\n';
  
  auto CPU_res_n = h.host_n;
  auto CPU_res_jx = h.host_jx;
  auto CPU_res_jy = h.host_jy;
  auto CPU_res_jz = h.host_jz;
  
  std::cout << "running GPU kernel\n";

  t.reset();
  for(int c=0;c<step_count;c++) {
    std::cout << c << ' ' << std::flush;
    if(!h.step_n(dt))
      std::cout << "failed to run n kernel\n";
    if(!h.step_dj(dt))
      std::cout << "failed to run dj kernel\n";
    if(!h.step_j(dt))
      std::cout << "failed to run j kernel\n";
    if(!h.advance_particles(dt))
      std::cout << "failed to run particles kernel\n";
    
    if(c%16==0) {
      h.get_particles();
      for(int id=0; id<h.np; id++)
        trajectories[id].push_back({h.host_particles[id], h.host_particles[id+h.np], h.host_particles[id+2*h.np]});
    }
#ifndef SIMPLEDRAW
    if(c%3)
      continue;
    h.get_data();
    for(int j=0;j<h.ny;j++){
      for(int i=0;i<h.nx;i++)
        n_2D_xy_slice[i+j*h.nx] = h.host_n[i+j*h.nx+(h.nz>>1)*h.nx*h.ny];
    }
    for(int j=0;j<h.ny;j++){
      for(int i=0;i<h.nx+1;i++)
        jx_2D_xy_slice[i+j*(h.nx+1)] = h.host_jx[i+j*(h.nx+1)+(h.nz>>1)*(h.nx+1)*h.ny];
    }
    for(int j=0;j<h.ny+1;j++){
      for(int i=0;i<h.nx;i++)
        jy_2D_xy_slice[i+j*h.nx] = h.host_jy[i+j*h.nx+(h.nz>>1)*h.nx*(h.ny+1)];
    }
    for(int j=0;j<h.ny;j++){
      for(int i=0;i<h.nx;i++)
        jz_2D_xy_slice[i+j*h.nx] = h.host_jz[i+j*h.nx+(h.nz>>1)*h.nx*h.ny];
    }
    fadey_draw(n_2D_xy_slice.data(), h.nx, h.ny, 0);
    fadey_draw(jx_2D_xy_slice.data(), h.nx+1, h.ny, 1);
    fadey_draw(jy_2D_xy_slice.data(), h.nx, h.ny+1, 2);
    fadey_draw(jz_2D_xy_slice.data(), h.nx, h.ny, 3);
#endif
  }
  std::cout << '\n';
  std::cout << "GPU time is " << t.elapsed() << " s\n";
  
  for(auto& v : h.host_n) v = .0;
  for(auto& v : h.host_jx) v = .0;
  for(auto& v : h.host_jy) v = .0;
  for(auto& v : h.host_jz) v = .0;
  
  h.get_data();
  for(int i=0;i<h.nx;i++)
    std::cout << h.host_n[i+(h.ny/2)*h.nx+h.nx*h.ny*(h.nz/2)] << ' ';
  std::cout << '\n';

  for(int i=0;i<h.nx+1;i++)
    std::cout << h.host_jx[i+(h.ny/2)*(h.nx+1)+(h.nx+1)*h.ny*(h.nz/2)] << ' ';
  std::cout << '\n';


  //calculate CPU vs GPU deltas:
  real delta_n = .0;
  for(int i=0;i<CPU_res_n.size();i++)
    delta_n += std::abs(h.host_n[i] - CPU_res_n[i]);
  delta_n /= (real) CPU_res_n.size();
  
  real delta_jx = .0;
  for(int i=0;i<CPU_res_jx.size();i++)
    delta_jx += std::abs(h.host_jx[i] - CPU_res_jx[i]);
  delta_jx /= (real) CPU_res_jx.size();
  
  real delta_jy = .0;
  for(int i=0;i<CPU_res_jy.size();i++)
    delta_jy += std::abs(h.host_jy[i] - CPU_res_jy[i]);
  delta_jy /= (real) CPU_res_jy.size();
  
  real delta_jz = .0;
  for(int i=0;i<CPU_res_jz.size();i++)
    delta_jz += std::abs(h.host_jz[i] - CPU_res_jz[i]);
  delta_jz /= (real) CPU_res_jz.size();
  
  std::cout << "n: " << delta_n << ", jx:" << delta_jx << ", jy:" << delta_jy << ", jz:" << delta_jz << '\n';

  fadey_close();

  std::ofstream of("trajectories.txt");
  of << "lines: trajectories\n";
  for(auto& t : trajectories) {
    for(int i=0;i<t.size()-1;i++)
      of << '(' << t[i][0] << ',' << t[i][1] << ',' << t[i][2] << ") (" << t[i+1][0] << ',' << t[i+1][1] << ',' << t[i+1][2] << ") (" << (real) i / (real) (t.size()-1) << ",0,0)\n";
  }
  of.close();
  
  return 0;
}

