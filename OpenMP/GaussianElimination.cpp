#include <stdio.h>
#include <sys/time.h>
#include <omp.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


#define ABS(x) (x < 0.0 ? -(x) : (x))
#define SWAP(a,b) do {double temp=a; a=b; b=temp;} while(0)

// nastavljanje spremenljivk okolja za koprocesor na gostitelju
// $ export MIC_ENV_PREFIX=MIC
// $ export MIC_OMP_NUM_THREADS=236
// $ export MIC_KMP_AFFINITY=scatter

//Funkcija za merjenje časa izvajanja
double dtime(){
        double tseconds = 0.0;
        struct timeval mytime;

        gettimeofday(&mytime, (struct timezone*) 0);
        tseconds = (double) (mytime.tv_sec + mytime.tv_usec * 1.0e-6);
        return (tseconds);

}

//Gaussova eliminacija z delnim pivotiranjem
//Pretvori matriko A v zgornje trikotno matriko
int gepp(double *A, double *B, int N){

#pragma offload target(mic) inout(A:length(N*N)) inout(B:length(N))
{
        __assume_aligned(A, 64);
        __assume_aligned(B, 64);

        for(int i=0;i<N-1;i++){

        //Najdimo vrstico z največjim elementom v trenutnem stolpcu
        int maxi=i;
        //#pragma ivdep
        //#pragma vector
            for(int j=i+1;j<N;j++)
                if (ABS(A[j*N+i])>ABS(A[maxi*N+i]))
					maxi=j;

        //Zamenjajmo vrstici
        #pragma vector
			for(int j=i;j<N;j++){
				SWAP(A[i*N+j],A[maxi*N+j]);
            }
            SWAP(B[maxi],B[i]);

		#pragma omp parallel for schedule(dynamic, 1)
        //Eliminacija
        for(int j=i+1;j<N;j++){
                        double tmp=A[j*N+i]/A[i*N+i];
                        #pragma vector
                        for(int k=i;k<N;k++){
                                A[j*N+k]-=(tmp*A[i*N+k]);
                        }
                        B[j]-=(tmp*B[i]);
                }
        }
}
        return 1;
}

//Vzvratna substitucija
//Resi sistem enacb oblike Ax=B
//A mora biti zgornje trikotna matrika
double backsubs(double *A, double *B, double *x, int N, double err){
{
        x[N-1]=B[N-1]/A[(N-1)*N+N-1];
        for(int i=N-2;i>=0;i--){
                double tmp=B[i];
                for(int j=i+1;j<N;j++)
                        tmp-=A[i*N+j]*x[j];

                x[i]=tmp/A[i*N+i];
        }

    //Preverimo resitev
    //Izracun povprecne kvadratne napake
        for(int i=0;i<N;i++){
                double bt=0.0;
                for(int j=0;j<N;j++)
                        bt+=A[i*N+j]*x[j];
                err+=((bt-B[i])*(bt-B[i]));
        }
}
        return sqrt(err/N);
}



//Stevilo enacb/neznank
int N=1024;

int main(void){

        int numthreads;

        #pragma offload target(mic) out(numthreads)
        #pragma omp parallel
        #pragma omp master
        numthreads = omp_get_num_threads();

        printf("Init %d threads\n", numthreads);

        //Rezervirajmo prostor za matrike
        double *A=(double *)_mm_malloc(sizeof(double)*N*N, 64);
        double *B=(double *)_mm_malloc(sizeof(double)*N, 64);
        double *x=(double *)_mm_malloc(sizeof(double)*N, 64);

		//Naključno napolnimo sistem enacb
        for(int i=0;i<N;i++){
                for(int j=0;j<N;j++)
                        A[i*N+j]=(double)rand()/RAND_MAX;
                B[i]=(double)rand()/RAND_MAX;
                x[i]=0.0;
        }

		//Gaussova eliminacija
        double tstart=dtime();
        gepp(A,B,N);
        double tstop=dtime();

		//Dokoncno resimo enacbo z vzvratno substitucijo
        double rmse=backsubs(A,B,x,N, 0.0);

        //Izpis
        printf("System of %d equations solved in %.2f s. RMSE= %.2f.\n",N,tstop)                                                                                                                                                             tstart,rmse);

        return 0;
}
