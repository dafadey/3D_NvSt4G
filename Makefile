all: test_flow test_flow_draw

COMPILE := g++ -g -DCL_HPP_MINIMUM_OPENCL_VERSION=100 -DCL_HPP_TARGET_OPENCL_VERSION=120 -O3 -fopenmp -c
	
CL_helpers.o: CL_helpers.cpp CL_helpers.h
	${COMPILE} $< -o $@
nvst.o: nvst.cpp CL_helpers.h data_type.h
	${COMPILE} $< -o $@
nvst_CPU.o: nvst_CPU.cpp data_type.h
	${COMPILE} $< -o $@
timer.o: timer.cpp timer.h
	${COMPILE} $< -o $@
	${COMPILE} $< -o $@
test_flow.o: test_flow.cpp CL_helpers.h data_type.h nvst_CPU.h timer.h simpledraw2D.h nvst.h
	${COMPILE} $< -o $@
test_flow_draw.o: test_flow.cpp CL_helpers.h data_type.h nvst_CPU.h timer.h simpledraw2D.h nvst.h
	${COMPILE} -DSIMPLEDRAW $< -o $@
simpledraw2d_glfw3.o: simpledraw2d_glfw3.cpp
	${COMPILE} -DSIMPLEDRAW $< -o $@
	
test_flow: CL_helpers.o nvst.o nvst_CPU.o test_flow.o timer.o
	g++ $^ -lOpenCL -fopenmp -o $@

test_flow_draw: CL_helpers.o nvst.o nvst_CPU.o test_flow_draw.o timer.o simpledraw2d_glfw3.o font.o
	g++ $^ -fopenmp -lOpenCL -lGL -lglfw -o $@

font.o: font.dat
	objcopy -I binary -O elf64-x86-64 --binary-architecture i386 $^ $@

#test_flow_draw: CL_helpers.o nvst.o nvst_CPU.o test_flow_draw.o timer.o
#	g++ $^ -fopenmp -lOpenCL -lGL -L/home/dan/tools/lib -lsimpledraw_glfw3 -Wl,-rpath=/home/dan/tools/lib -o $@

clean:
	rm -f *.o test_flow test_flow_draw
