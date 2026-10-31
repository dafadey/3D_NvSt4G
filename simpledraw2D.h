#ifdef SIMPLEDRAW
void fadey_init(int, int, int); // size X, size Y, number of tiles
void fadey_draw(float* , int , int , int ); // data, size X, size Y, number tile to update
void fadey_draw(double* , int , int , int ); // data, size X, size Y, number tile to update
void fadey_draw_particles(int, float* , int, int, double=1.0, double=1.0, double=1.0); //index, data (x,y format), number of particles, number tile to update, color (bounds are calculated automatically)
void fadey_draw_particles_reset_bounds(int); //resets bounds for this tile

void fadey_draw_1D(int, float* , int, int, double=1.0, double=1.0, double=1.0); // index, data, size X, number tile to update, color
void fadey_close(); // call on finish
#else
void fadey_init(int, int, int) {} // size X, size Y, number of tiles
void fadey_draw(float* , int , int , int ) {} // data, size X, size Y, number tile to update
void fadey_draw(double* , int , int , int ) {} // data, size X, size Y, number tile to update
void fadey_draw_particles(int, float* , int, int, double=1.0, double=1.0, double=1.0) {} //index, data (x,y format), number of particles, number tile to update, color (bounds are calculated automatically)
void fadey_draw_particles_reset_bounds(int) {} //resets bounds for this tile

void fadey_draw_1D(int, float* , int, int, double=1.0, double=1.0, double=1.0) {} // index, data, size X, number tile to update, color
void fadey_close() {} // call on finish

#endif
