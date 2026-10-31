#### about
Out of the box this is a pure Linux project.
It can be adopted to any system compatible with OpenCL (and OpenGL if you need visualization) by the means of your personal efforts.

#### build prerequisites:
- OpenCL with appropriate drivers for your device (AMD, nVidia, Intel discreet or embedded GPU)
- Optionally OpenGL and glfw3 library if you need visualization

Console build:
```bash
make test_flow
```
Run 404 iterations:
```bash
LD_LIBRARY_PATH=./ ./test_flow 404
```

Visual build:
```bash
make test_flow_draw
```
Run:
```bash
LD_LIBRARY_PATH=./ ./test_flow_draw 404
```
