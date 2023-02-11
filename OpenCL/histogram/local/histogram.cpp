#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

//Stevilo elementov
#define N 100000000

//Sirina histograma
#define BINS 256

#define WORKGROUP_SIZE	256
#define GLOBAL_SIZE	16128
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
    int i;
	int n = N;
	cl_int ret;

	double start;
	start=dtime();

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
	int *randomT = (int *)malloc(N*sizeof(int));
    int *histogram = (int *)calloc(BINS,sizeof(int));

    //Napolnimo tabelo z nakljucnimi stevili
	for(i=0;i<N;i++){
		randomT[i]=rand()%256;
	}
 
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

	// Delitev dela
    size_t local_item_size = WORKGROUP_SIZE;
	//size_t num_groups = ((N-1)/local_item_size+1);
    size_t global_item_size = GLOBAL_SIZE;//num_groups*local_item_size;	

    // Alokacija pomnilnika na napravi
    cl_mem t_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
									  N*sizeof(int), randomT, &ret);
			// kontekst, nacin, koliko, lokacija na hostu, napaka	
    cl_mem h_mem_obj = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
									  BINS*sizeof(int), histogram, &ret);
  
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
			// maksimalna dolzina niza, kazalec na niz, dejanska dolzina niza
	build_log =(char *)malloc(sizeof(char)*(build_log_len+1));
	ret = clGetProgramBuildInfo(program, device_id[0], CL_PROGRAM_BUILD_LOG, 
							    build_log_len, build_log, NULL);
	printf("%s\n", build_log);
	free(build_log);

    // scepec: priprava objekta
    cl_kernel kernel = clCreateKernel(program, "histogram", &ret);
			// program, ime scepca, napaka
 
    // scepec: argumenti
    ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&t_mem_obj);
    ret |= clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&h_mem_obj);
	ret |= clSetKernelArg(kernel, 2, BINS*sizeof(int), NULL);
    ret |= clSetKernelArg(kernel, 3, sizeof(cl_int), (void *)&n);
			// scepec, stevilka argumenta, velikost podatkov, kazalec na podatke

	printf("-time-: %.2f s\n", dtime()-start);

	// scepec: zagon
    ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL,						
								 &global_item_size, &local_item_size, 0, NULL, NULL);	
			// vrsta, scepec, dimenzionalnost, mora biti NULL, 
			// kazalec na stevilo vseh niti, kazalec na lokalno stevilo niti, 
			// dogodki, ki se morajo zgoditi pred klicem
																						
    // Kopiranje rezultatov histograma nazaj
    ret = clEnqueueReadBuffer(command_queue, h_mem_obj, CL_TRUE, 0, BINS*sizeof(int), histogram, 0, NULL, NULL);				
			// branje v pomnilnik iz naparave, 0 = offset
			// zadnji trije - dogodki, ki se morajo zgoditi prej

	for(i = 0;i < BINS;i ++){
		printf("%d ", histogram[i]);
	}
			
    // Prikaz rezultatov
	printf("\nTime to build histogram from %d samples: %.2f s.\n",N,dtime()-start);
 
    // ciscenje
    ret = clFlush(command_queue);
    ret = clFinish(command_queue);
    ret = clReleaseKernel(kernel);
    ret = clReleaseProgram(program);
    ret = clReleaseMemObject(t_mem_obj);
    ret = clReleaseMemObject(h_mem_obj);
    ret = clReleaseCommandQueue(command_queue);
    ret = clReleaseContext(context);
	
    free(randomT);
    free(histogram);

    return 0;
}
