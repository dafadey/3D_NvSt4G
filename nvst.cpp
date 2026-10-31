#include "CL_helpers.h"
#include "data_type.h"
#include "nvst.h"

#include <fstream>
#include <cmath>
#include <iostream>

bool nvst::init() {
  ocl = new opencl;
  ocl_data = new opencl_data<real>(*ocl);
  if(!ocl->buildOCLsources("nvst_kernels.cl")) {
    std::cerr << "ERROR building CL sources\n";
    return false;
  }
  if(!ocl->add_kernel("do_n")) {
    std::cerr << "ERROR adding kernel\n";
    return false;
  }
  if(!ocl->add_kernel("do_dj")) {
    std::cerr << "ERROR adding kernel\n";
    return false;
  }
  if(!ocl->add_kernel("do_j")) {
    std::cerr << "ERROR adding kernel\n";
    return false;
  }
  if(!ocl->add_kernel("advance_particles")) {
    std::cerr << "ERROR adding kernel\n";
    return false;
  }
  if(nx && ny && nz)
    std::cout << "allocating the memory\n";
  else {
    std::cout << "one of dimensions is zero. no memory allocated, set your dimensions first!\n";
    return false;
  }
  n = cl::Buffer(ocl_data->alloc(nx*ny*nz*sizeof(real), opencl_data<real>::memType::RW));
  jx = cl::Buffer(ocl_data->alloc((nx+1)*ny*nz*sizeof(real), opencl_data<real>::memType::RW));
  djx = cl::Buffer(ocl_data->alloc((nx+1)*ny*nz*sizeof(real), opencl_data<real>::memType::RW));
  jy = cl::Buffer(ocl_data->alloc(nx*(ny+1)*nz*sizeof(real), opencl_data<real>::memType::RW));
  djy = cl::Buffer(ocl_data->alloc(nx*(ny+1)*nz*sizeof(real), opencl_data<real>::memType::RW));
  jz = cl::Buffer(ocl_data->alloc(nx*ny*(nz+1)*sizeof(real), opencl_data<real>::memType::RW));
  djz = cl::Buffer(ocl_data->alloc(nx*ny*(nz+1)*sizeof(real), opencl_data<real>::memType::RW));
  particles = cl::Buffer(ocl_data->alloc(np*3*sizeof(real), opencl_data<real>::memType::RW));
  
  host_n.resize(nx*ny*nz, 0);
  host_jx.resize((nx+1)*ny*nz, 0);
  host_jy.resize(nx*(ny+1)*nz, 0);
  host_jz.resize(nx*ny*(nz+1), 0);
  host_djx.resize((nx+1)*ny*nz, 0);
  host_djy.resize(nx*(ny+1)*nz, 0);
  host_djz.resize(nx*ny*(nz+1), 0);
  host_particles.resize(np*3, 0);
      
  dx_1 = real(nx)/Lx;
  dy_1 = real(ny)/Ly;
  dz_1 = real(nz)/Lz;
  
  return true;
}

nvst::~nvst()
{
  if(ocl_data)
    delete ocl_data;
  if(ocl)
    delete ocl;
}


void nvst::set_data() {
  ocl_data->h2d(host_n, n);
  ocl_data->h2d(host_jx, jx);
  ocl_data->h2d(host_jx, djx);
  ocl_data->h2d(host_jy, jy);
  ocl_data->h2d(host_jy, djy);
  ocl_data->h2d(host_jz, jz);
  ocl_data->h2d(host_jz, djz);
  ocl_data->h2d(host_particles, particles);
}

void nvst::get_data() {
  ocl_data->d2h(n, host_n.data(), host_n.size());
  ocl_data->d2h(jx, host_jx.data(), host_jx.size());
  ocl_data->d2h(jy, host_jy.data(), host_jy.size());
  ocl_data->d2h(jz, host_jz.data(), host_jz.size());
}

void nvst::get_particles() {
  ocl_data->d2h(particles, host_particles.data(), host_particles.size());
}

bool nvst::step_n(real dt) {
  if(!step_n_kernel) {
    std::cout << "setting up n kernel\n";
    step_n_kernel = ocl_data->select_kernel("do_n");
    ocl_data->set_arg<unsigned int>(0, nx);
    ocl_data->set_arg<unsigned int>(1, ny);
    ocl_data->set_arg<unsigned int>(2, nz);
    ocl_data->set_arg<real>(3, dx_1);
    ocl_data->set_arg<real>(4, dy_1);
    ocl_data->set_arg<real>(5, dz_1);
    ocl_data->set_arg<real>(6, dt);
    ocl_data->set_arg<cl::Buffer>(7, jx);
    ocl_data->set_arg<cl::Buffer>(8, jy);
    ocl_data->set_arg<cl::Buffer>(9, jz);
    ocl_data->set_arg<cl::Buffer>(10, n);
  }
  ocl_data->current_kernel_ptr = step_n_kernel;
  return ocl_data->run(WG, WF);
}

bool nvst::step_dj(real dt) {
  if(!step_dj_kernel) {
    std::cout << "setting up dj kernel\n";
    step_dj_kernel = ocl_data->select_kernel("do_dj");
    ocl_data->set_arg<unsigned int>(0, nx);
    ocl_data->set_arg<unsigned int>(1, ny);
    ocl_data->set_arg<unsigned int>(2, nz);
    ocl_data->set_arg<real>(3, dx_1);
    ocl_data->set_arg<real>(4, dy_1);
    ocl_data->set_arg<real>(5, dz_1);
    ocl_data->set_arg<cl::Buffer>(6, n);
    ocl_data->set_arg<cl::Buffer>(7, jx);
    ocl_data->set_arg<cl::Buffer>(8, jy);
    ocl_data->set_arg<cl::Buffer>(9, jz);
    ocl_data->set_arg<cl::Buffer>(10, djx);
    ocl_data->set_arg<cl::Buffer>(11, djy);
    ocl_data->set_arg<cl::Buffer>(12, djz);
    ocl_data->set_arg<real>(13, T0);
    ocl_data->set_arg<real>(14, R);
  }
  ocl_data->current_kernel_ptr = step_dj_kernel;
  return ocl_data->run(WG, WF);
}
            
bool nvst::step_j(real dt) {
  if(!step_j_kernel) {
    std::cout << "setting up j kernel\n";
    step_j_kernel = ocl_data->select_kernel("do_j");
    ocl_data->set_arg<unsigned int>(0, nx);
    ocl_data->set_arg<unsigned int>(1, ny);
    ocl_data->set_arg<unsigned int>(2, nz);
    ocl_data->set_arg<real>(3, dt);
    ocl_data->set_arg<cl::Buffer>(4, jx);
    ocl_data->set_arg<cl::Buffer>(5, jy);
    ocl_data->set_arg<cl::Buffer>(6, jz);
    ocl_data->set_arg<cl::Buffer>(7, djx);
    ocl_data->set_arg<cl::Buffer>(8, djy);
    ocl_data->set_arg<cl::Buffer>(9, djz);
  }
  ocl_data->current_kernel_ptr = step_j_kernel;
  return ocl_data->run(WG, WF);
}
                                
bool nvst::advance_particles(real dt) {
  if(!advance_particles_kernel) {
    std::cout << "setting up j kernel\n";
    advance_particles_kernel = ocl_data->select_kernel("advance_particles");
    ocl_data->set_arg<cl::Buffer>(0, particles);
    ocl_data->set_arg<unsigned int>(1, np);
    ocl_data->set_arg<unsigned int>(2, nx);
    ocl_data->set_arg<unsigned int>(3, ny);
    ocl_data->set_arg<unsigned int>(4, nz);
    ocl_data->set_arg<real>(5, Lx);
    ocl_data->set_arg<real>(6, Ly);
    ocl_data->set_arg<real>(7, Lz);
    ocl_data->set_arg<real>(8, dt);
    ocl_data->set_arg<cl::Buffer>(9, jx);
    ocl_data->set_arg<cl::Buffer>(10, jy);
    ocl_data->set_arg<cl::Buffer>(11, jz);
    ocl_data->set_arg<cl::Buffer>(12, n);
  }
  ocl_data->current_kernel_ptr = advance_particles_kernel;
  return ocl_data->run(WG, WF);
}
