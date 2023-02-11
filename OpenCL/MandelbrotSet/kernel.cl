__kernel void mandelbort (__global int *image, int width, int height, int max_gray, volatile __global int *ploscina)						
{
	float x0,y0,x,y,xtemp;
	int i,j;
	int color;
	int iter;
	int max_iteration=1000; 
												
	// globalni indeks elementa
	i = get_global_id(0);
	j = get_global_id(1);

	if(i < height && j < width){ //preverimo ce je nit zunaj slike
		x0 = (float)j/width*(float)3.5-(float)2.5; //zacetna vrednost
		y0 = (float)i/height*(float)2.0-(float)1.0;
		x = 0;
		y = 0;
		iter = 0;
		while((x*x+y*y <= 4) && (iter < max_iteration)) //ne sme se priblizati neskoncnosti v N-iteracijah
		{ 
			//ponavljamo, dokler ne izpolnemo enega izmed pogojev
			xtemp = x*x-y*y+x0;
			y = 2*x*y+y0;
			x = xtemp;
			iter++;
		}
		color = (int)(iter/(float)max_iteration*(float)max_gray); //pobarvamo piksel z ustrezno barvo
		if(color == 255){
				atomic_add(ploscina, 1);
		}
		image[i*width+j] = max_gray-color;
	}

}	