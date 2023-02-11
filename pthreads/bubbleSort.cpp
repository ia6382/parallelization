#include<stdio.h>
#include<pthread.h>
#include<stdlib.h>
#include<math.h>
#include<time.h>
#include<string.h>

//Število elementov v tabeli
#define N 20000

//Število niti
#define NTHREADS 2

//Zamenjaj elementa
#define SWAP(a,b) do {int temp=a; a = b; b = temp;} while(0)

//Struktura za prenasanje argumentov funkciji za urejanje
struct params{
	int rank;	//indeks niti
	int *L;		//kazalec na seznam		
};

//Funkcija za primerjavo dveh elementov (test s qsort())
int compare (const void * a, const void * b)
{
  return ( *(int*)a - *(int*)b );
}

void * bubbleSort(void *arg){
	int *L= ((params *)arg)->L;
	int rank=((params *)arg)->rank;
	
	//Razdelitev tabele na podsezname
	//Izracunajmo zgornjo in spodnjo mejo podseznama za dano nit (vkljucno)
	int m=(rank*N)/NTHREADS;
	int M=((rank+1)*N)/NTHREADS-1;

	int sorted=0;
	//Glavna zanka
	while(!sorted){

		sorted=1;
		//Veliko število navzgor
		for(int i=m; i<M; i++)
			if (L[i] > L[i+1]){
				SWAP(L[i], L[i+1]);
				sorted = 0;
			}
		//Ce smo že uredili koncamo
		if(sorted) break;

		sorted=1;
		//Ucinkoviteje, ce zacnemo kar na isti strani seznama
		//Majhno stevilo navzdol
		for(int i=M; i>m; i--)
			if (L[i-1] > L[i]){
				SWAP(L[i], L[i-1]);
				sorted = 0;
			}
		
	}
	return NULL;
}


int main(int argc, char **argv){

	//Prostor za niti in argumente
	pthread_t t[NTHREADS];
	params p[NTHREADS];

	//Zasedemo prostor za seznam neurejenih stevil
	int *L=(int *)malloc(N*sizeof(int));

	//Napolnimo z nakljucnimi števili
	srand(time(NULL));
	for(int i=0;i<N;i++){
		L[i]=rand();
	}
	
	//Zasedemo prostor za referenèni seznam in prepisemo vanj vrednosti (za preverjanje)
	int *Lref=(int *)malloc(N*sizeof(int));
	memcpy(Lref,L,N*sizeof(int));

	//Spremenljivke za merjenje casa izvajanja
	clock_t ts1,ts2,te1,te2;

	//Merimo cas BubbleSort
	ts1=clock();

	//Ustvarimo niti
	for(int i=0; i < NTHREADS;i++){
		p[i].L=L;
		p[i].rank=i;
		pthread_create(&t[i],NULL,bubbleSort,(void *)&p[i]);
	}

	//Pocakamo na vse niti
	for(int i=0; i<NTHREADS; i++)
		pthread_join(t[i], NULL);
	
	te1=clock();
	
	//Zazenemo qsort nad enakimi stevili
	ts2=clock();
	qsort(Lref,N,sizeof(int),compare);
	te2=clock();

	//Preverjanje rezultatov
	int test=1;
	for(int i=0;i<N;i++)
		if(L[i]!=Lref[i]){
			printf("Error, check fail! Index: %d; Value: %d; True value: %d\n",i,L[i],Lref[i]);
			test=0;
			break;
		}

	//Izpis casov
	if(test){
		printf("Test passed!\n");
		printf("Time Qicksort: %f s\n",(double)(te2 - ts2) / CLOCKS_PER_SEC);
		printf("Time Bubblesort: %f s\n",(double)(te1 - ts1) / CLOCKS_PER_SEC);
	}



	return 0;
}