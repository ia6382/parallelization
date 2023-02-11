__kernel void histogram (__global const int *randomT,
						 __global int *histogram,		
						 __local int *lHistogram,
						 int N)
{	
	int lid = get_local_id(0);
    int gid = get_global_id(0);
	int i = gid;
	int tmp = 0;
	
	while(i < N){
		//pristejemo globalne vrednosti v lokalni histogram
		tmp = randomT[i];
		atomic_add(&lHistogram[tmp], 1);

		i += get_global_size(0);
	}

	//sinhroniziramo niti
	barrier( CLK_LOCAL_MEM_FENCE );
	
	//pristejemo lokalni histogram globalnemu
	if(lid < 256){
		atomic_add(&histogram[lid], lHistogram[lid]);
	}

}				
