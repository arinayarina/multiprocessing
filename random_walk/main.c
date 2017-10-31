#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <omp.h>
#include <assert.h>

typedef struct scalar_ctx_t {
	int a;    // начало отрезка
	int b;    // конец отрезка
	int x;    // начальная точка
	int N;    // количество частиц
	double p; // вероятность
	int P;    // число потоков
} scalar_ctx_t;

void random_walk(void *context, FILE *f)
{
	scalar_ctx_t *ctx = context;
	int success = 0; // количество дошедших до b
	int sum_live = 0;
	
	struct timeval start, end;
	assert(gettimeofday(&start, NULL) == 0);
	
	int * seed = (int*) malloc(ctx->N * sizeof(int));
		int s = (int)(time(NULL));
		for(int i = 0; i < ctx->N; i++) {
			seed[i] = rand_r(&s);
	}
	#pragma omp parallel for reduction (+: sum_live)
	for (int i = 0; i < ctx->N; i++) {
		int x = ctx->x;
		int new_seed = seed[i];
		while (1) {
			sum_live++;
			if (x == ctx->a) {
				break;
			} else
			if (x == ctx->b) {
				#pragma omp atomic
				success++;
				break;
			}
			double rand_num = rand_r(&new_seed);
			double choice = (rand_num / (RAND_MAX));
			if (choice <= ctx->p) {
				x += 1;
			} else {
				x -= 1;
			}
		}
	}
	assert(gettimeofday(&end, NULL) == 0);
	double delta = ((end.tv_sec - start.tv_sec) * 1000000u + end.tv_usec - start.tv_usec) / 1.e6;
	
	double prob = (double)(success) / (double)(ctx->N);
	double mean_live = (double)(sum_live) / (double)(ctx->N);
	fprintf(f, "%f %f %f %d %d %d %d %f %d\\\n", prob, mean_live, delta, ctx->a, ctx->b, ctx->x, ctx->N, ctx->p, ctx->P);
}

int main(int argc, char **argv) {
	if( argc == 7 ) {
		int a = atoi(argv[1]);
		int b = atoi(argv[2]);
		int x = atoi(argv[3]);
		int N = atoi(argv[4]);
		double p = atof(argv[5]);
		int P = atoi(argv[6]);
		
		scalar_ctx_t ctx = {
			.a = a,
			.b = b,
			.x = x,
			.N = N,
			.p = p,
			.P = P,
		};
		
		FILE *f = fopen("data.txt", "w");
		if (f != NULL) {
			ctx.P = 1;
			for( int i = 500; i < 30000; i+=500 ) {
				ctx.N = i;
				random_walk(&ctx, f);
			}
			ctx.P = 4;
			for( int i = 500; i < 30000; i+=500 ) {
				ctx.N = i;
				random_walk(&ctx, f);
			}
		}
		fclose(f);
	}
	return 0;
}