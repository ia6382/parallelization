# parallelization
Examples of parallelization for GPU with OpenCL as well as for CPU using pthreads and OpenMP. Homework for Parallel Systems course at the University of Ljubljana, Faculty of Computer and Information Science in 2016. Written in C++.

## OpenCL
Two examples parallelized for a GPU using OpenCL. OpenCL is a framework for writing programs that execute across heterogeneous platforms. It allows developers to write kernels that can be computed in parallel on a GPU.
* **MandelbrotSet**: draws and calculates the surface area of the [Mandelbrot set](https://en.wikipedia.org/wiki/Mandelbrot_set). A pgm.h file contains a library to write the grayscale image. 
* **histogram**: calculates a histogram from random data. There are two versions: one that uses only global memory and another that also uses the local memory (registers) of the GPU. The latter version proves to be faster as we compute parts of histograms locally and thus avoid most of the unaligned global memory access which slows the performance.

## pthreads
Parallelization of a bubble sort algorithm for multiple threads on a CPU. 
Included library Pthreads-win32 is an Open Source Software implementation of the Threads POSIX execution model.

## OpenMP
A parallelisation of Gaussian elimination. Tested on the [XeonPhi](https://en.wikipedia.org/wiki/Xeon_Phi) manycore processor.
The OpenMP API supports multi-platform shared-memory parallel programming.
