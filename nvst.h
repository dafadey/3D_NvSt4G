#pragma once
#include "CL_helpers.h"
#include "data_type.h"

struct nvst {
  opencl* ocl{};
  opencl_data<real>* ocl_data{};
  
  bool init();

  ~nvst();

  int nx{111}, ny{111}, nz{111}, np{11}; // number of n cells along each dim, thus currents are +1 bigger in size along corresponding dim (e.g. jx has dims of [nz][ny][nx+1]
  real Lx{1.}, Ly{1.}, Lz{1.};
  real dx_1, dy_1, dz_1;
  real T0{1.}, R{1.};
  cl::Buffer n, jx, jy, jz, djx, djy, djz, particles;
  std::vector<real> host_n, host_jx, host_jy, host_jz, host_djx, host_djy, host_djz, host_particles;
  cl::Kernel* step_n_kernel{};
  cl::Kernel* step_dj_kernel{};
  cl::Kernel* step_j_kernel{};
  cl::Kernel* advance_particles_kernel{};
  
  void set_data();
  
  void get_data();
  void get_particles();
  
  bool step_n(real dt);

  bool step_dj(real dt);
              
  bool step_j(real dt);
  
  bool advance_particles(real dt);
};
