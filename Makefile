all: nvst.so test_flow test_flow_draw

COMPILE := g++ -fPIC -g -DCL_HPP_MINIMUM_OPENCL_VERSION=100 -DCL_HPP_TARGET_OPENCL_VERSION=120 -O3 -fopenmp -c
	
CL_helpers.o: CL_helpers.cpp CL_helpers.h
	${COMPILE} $< -o $@
nvst.o: nvst.cpp CL_helpers.h data_type.h
	${COMPILE} $< -o $@
nvst_CPU.o: nvst_CPU.cpp data_type.h
	${COMPILE} $< -o $@
timer.o: timer.cpp timer.h
	${COMPILE} $< -o $@
	
test_flow.o: test_flow.cpp CL_helpers.h data_type.h nvst_CPU.h timer.h simpledraw2D.h nvst.h
	${COMPILE} $< -o $@
test_flow_draw.o: test_flow.cpp CL_helpers.h data_type.h nvst_CPU.h timer.h simpledraw2D.h nvst.h
	${COMPILE} -DSIMPLEDRAW $< -o $@
simpledraw2d_glfw3.o: simpledraw2d_glfw3.cpp
	${COMPILE} -DSIMPLEDRAW $< -o $@
	
font.o: font.dat
	objcopy -I binary -O elf64-x86-64 --binary-architecture i386 $^ $@

nvst.so: CL_helpers.o nvst.o nvst_CPU.o
	g++ -fPIC -shared $^ -o $@
	
test_flow: test_flow.o timer.o nvst.so
	g++ -fPIC $^ -lOpenCL -fopenmp -o $@

test_flow_draw: test_flow_draw.o timer.o nvst.so simpledraw2d_glfw3.o font.o
	g++ -fPIC $^ -fopenmp -lOpenCL -lGL -lglfw -o $@

clean:
	rm -f *.o nvst.so test_flow test_flow_draw
