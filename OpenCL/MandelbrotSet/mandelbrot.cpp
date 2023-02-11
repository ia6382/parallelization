#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include "pgm.h"

#define SIZE			1024
#define WORKGROUP_SIZE	16
#define MAX_SOURCE_SIZE	16384

double dtime(){
	double tseconds = 0.0;
	struct timeval mytime;

	gettimeofday(&mytime, (struct timezone*) 0);
	tseconds = (double) (mytime.tv_sec + mytime.tv_usec * 1.0e-6);
	return (tseconds);

}

int main(void) 
{
	double start;
	PGMData slika;
	
	int height = SIZE;
	int width = SIZE;
	int max_gray=255;

	start=dtime();

	cl_int ret;

    // Branje datoteke
    FILE *fp;
    char *source_str;
    size_t source_size;

    fp = fopen("kernel.cl", "r");
    if (!fp) 
	{
		fprintf(stderr, ":-(#\n");
        exit(1);
    }
    source_str = (char*)malloc(MAX_SOURCE_SIZE);
    source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
	source_str[source_size] = '\0';
    fclose( fp );
	
	// Rezervacija pomnilnika
	int *image=(int *)malloc(sizeof(int)*width*height);
	int *ploscina = (int *)malloc(sizeof(int));
	
    // Podatki o platformi
    cl_platform_id	platform_id[10];
    cl_uint			ret_num_platforms;
	char			*buf;
	size_t			buf_len;
	ret = clGetPlatformIDs(10, platform_id, &ret_num_platforms);
			// max. stevilo platform, kazalec na platforme, dejansko stevilo platform
	
	// Podatki o napravi
	cl_device_id	device_id[10];
	cl_uint			ret_num_devices;
	
	// Delali bomo s platform_id[0] na GPU
	ret = clGetDeviceIDs(platform_id[0], CL_DEVICE_TYPE_GPU, 10,	
						 device_id, &ret_num_devices);				
			// izbrana platforma, tip naprave, koliko naprav nas zanima
			// kazalec na naprave, dejansko stevilo naprav

    // Kontekst
    cl_context context = clCreateContext(NULL, 1, &device_id[0], NULL, NULL, &ret);
			// kontekst: vkljucene platforme - NULL je privzeta, stevilo naprav, 
			// kazalci na naprave, kazalec na call-back funkcijo v primeru napake
			// dodatni parametri funkcije, stevilka napake
 
    // Ukazna vrsta
    cl_command_queue command_queue = clCreateCommandQueue(context, device_id[0], 0, &ret);
			// kontekst, naprava, INORDER/OUTOFORDER, napake

   
	ploscina[0] = 0;

	// Alokacija pomnilnika na napravi
    cl_mem mem_obj = clCreateBuffer(context, CL_MEM_READ_WRITE,
									  height*width*sizeof(int), image, &ret);
			// kontekst, nacin, koliko, lokacija na hostu, napaka
	cl_mem ploscina_obj = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
									  sizeof(int), ploscina, &ret);
  
    // Priprava programa
    cl_program program = clCreateProgramWithSource(context,	1, (const char **)&source_str,  
												   NULL, &ret);
			// kontekst, stevilo kazalcev na kodo, kazalci na kodo,		
			// stringi so NULL terminated, napaka													
 
    // Prevajanje
    ret = clBuildProgram(program, 1, &device_id[0], NULL, NULL, NULL);
			// program, stevilo naprav, lista naprav, opcije pri prevajanju,
			// kazalec na funkcijo, uporabniski argumenti

	// Log
	size_t build_log_len;
	char *build_log;
	ret = clGetProgramBuildInfo(program, device_id[0], CL_PROGRAM_BUILD_LOG, 
								0, NULL, &build_log_len);
			// program, naprava, tip izpisa, 
			// maksimalna dolzina niza, kazalec na niz, dejanska dol"zina niza
	
	build_log =(char *)malloc(sizeof(char)*(build_log_len+1));
	ret = clGetProgramBuildInfo(program, device_id[0], CL_PROGRAM_BUILD_LOG, 
							    build_log_len, build_log, NULL);
	printf("%s\n", build_log);
	free(build_log);
 
    // scepec: priprava objekta
    cl_kernel kernel = clCreateKernel(program, "mandelbrot", &ret);
			// program, ime scepca, napaka
 
    // scepec: argumenti
    clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&mem_obj);
    clSetKernelArg(kernel, 1, sizeof(cl_int), (void *)&height);
    clSetKernelArg(kernel, 2, sizeof(cl_int), (void *)&width);
    clSetKernelArg(kernel, 3, sizeof(cl_int), (void *)&max_gray);
    clSetKernelArg(kernel, 4, sizeof(cl_mem), (void *)&ploscina_obj);
			// scepec, stevilka argumenta, velikost podatkov, kazalec na podatke

	// Delitev dela
	int stBlokovH = (height/WORKGROUP_SIZE) + 1;
	int stBlokovW = (width/WORKGROUP_SIZE) + 1;
	
	size_t local_item_size[2] = {WORKGROUP_SIZE, WORKGROUP_SIZE};
	size_t global_item_size[2] = {WORKGROUP_SIZE*stBlokovH, WORKGROUP_SIZE*stBlokovW};

	// scepec: zagon
    ret = clEnqueueNDRangeKernel(command_queue, kernel, 2, NULL,						
								 global_item_size, local_item_size, 0, NULL, NULL);	
			// vrsta, scepec, dimenzionalnost, mora biti NULL, 
			// kazalec na stevilo vseh niti, kazalec na lokalno stevilo niti, 
			// dogodki, ki se morajo zgoditi pred klicem
																						
    // Kopiranje rezultatov, slike in ploscine nazaj
    ret = clEnqueueReadBuffer(command_queue, mem_obj, CL_TRUE, 0,						
							  height*width*sizeof(int), image, 0, NULL, NULL);	
	ret = clEnqueueReadBuffer(command_queue, ploscina_obj, CL_TRUE, 0,						
							 sizeof(int), ploscina, 0, NULL, NULL);
			// branje v pomnilnik iz naparave, 0 = offset
			// zadnji trije - dogodki, ki se morajo zgoditi prej


	int vse = height*width;
	double procentov = (ploscina[0]*100)/vse;
	double realno = (3.5*2)*(procentov/100);
 
 
 	printf("Time GPU: %.2f s, PLOSCINA: %.2f\n",dtime()-start, realno);
	
	slika.height=height;
	slika.width=width;
	slika.max_gray=max_gray;
	slika.image=image;
	
	writePGM("mandelbrot.pgm",&slika);
	
	
    // ciscenje
    ret = clFlush(command_queue);
    ret = clFinish(command_queue);
    ret = clReleaseKernel(kernel);
    ret = clReleaseProgram(program);
    ret = clReleaseMemObject(mem_obj);
    ret = clReleaseCommandQueue(command_queue);
    ret = clReleaseContext(context);
	

    return 0;
}
													

