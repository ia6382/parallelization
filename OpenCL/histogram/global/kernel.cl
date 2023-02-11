__kernel void histogram (__global const int *randomT,
						 __global int *histogram,		
						 int N
						)
{	
	int lid = get_local_id(0);
    int gid = get_global_id(0);
	int i = gid;
	
	while(i < N){
		//pristejemo globalne vrednosti v globalni histogram
		atomic_add(&histogram[randomT[i]], 1);

		i += get_global_size(0);
	}
}	