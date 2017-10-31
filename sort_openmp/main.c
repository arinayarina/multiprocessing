#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <omp.h>
#include <assert.h>

typedef struct scalar_ctx_t {
	int n;           // число элементов
	int m;           // максимальный размер чанка
	int P;           // число потоков
	int *data;       // изначальный массив
	int *sorted;     // отсортированный массив
	double worktime; // время работы
} scalar_ctx_t;

int max (int a, int b) {
    if (a < b) {
        return b;
    }
    else {
        return a;
    }
}

int binary_search (int x, int* arr, int p, int r) {
    int low = p;
    int high = max(p, r + 1);
    while (low < high) {
        int mid = (low + high) / 2;
        if (x <= arr[mid]) {
            high = mid;
        }
        else {
            low = mid + 1;
        }
    }
    return high;
}

int cmpfunc (const void * a, const void * b) {
    return ( *(int*)a - *(int*)b );
}

void simple_merge (int* input, int left1, int right1, int left2, int right2, int* output, int t) {
	int i = 0, j = 0;
    while ((i + left1 < right1 + 1) && (j + left2 < right2 + 1)) {
        if (input[left1 + i] < input[left2 + j]) {
            output[t + i + j] = input[left1 + i];
            i++;
        } else {
            output[t + i + j] = input[left2 + j];
            j++;
        }
    }
	
    for (; i + left1 < right1 + 1; i++)
        output[t + i + j] = input[left1 + i];
	
    for (; j + left2 < right2 + 1; j++)
        output[t + i + j] = input[left2 + j];
}

void swap (int* a, int* b) {
	int c = *a;
	*a = *b;
	*b = c;
}

void parallel_merge (int* input, int left1, int right1, int left2, int right2, int* output, int t) {
    int n1 = right1 - left1 + 1;
    int n2 = right2 - left2 + 1;
    if (n1 < n2) {
        swap(&left1, &left2);
        swap(&right1, &right2);
        swap(&n1, &n2);
    }
    if (n1 == 0) {
        return;
    }
    else { 
        int q1 = (left1 + right1) / 2;
        int q2 = binary_search(input[q1], input, left2, right2); 
	int q3 = t + (q1 - left1) + (q2 - left2);
        output[q3] = input[q1];
		
        #pragma omp parallel
        {
            #pragma omp sections nowait
            {
                #pragma omp section
                {
                    simple_merge(input, left1, q1 - 1, left2, q2 - 1, output, t);
                }
                #pragma omp section
                {
                    simple_merge(input, q1 + 1, right1, q2, right2, output, q3 + 1);
                }
            }
        }
    }
}

void parallel_merge_sort (int* input, int l, int r, int* output, int s, int chunk_size) {
    int n = r - l + 1;
    if (r - l <= chunk_size) {
        qsort(&input[l], r - l + 1, sizeof(int), cmpfunc);
        memcpy(&output[s], &input[l], (r - l + 1) * sizeof(int));
        return;
    }
    int* temp =  (int *)malloc(sizeof(int) * n);
    int q = (l + r) / 2;
    int t = q - l + 1; 
    #pragma omp parallel
    {
        #pragma omp sections nowait
        {
            #pragma omp section
            {
                parallel_merge_sort(input, l, q, temp, 0, chunk_size);
            }
                
            #pragma omp section
            {
                parallel_merge_sort(input, q + 1, r, temp, t, chunk_size);
            }
        }
    }
    parallel_merge(temp, 0, t - 1, t, n - 1, output, s);
    free(temp);
}

void merge_sort (void *context, FILE *stats, FILE *data) {
	scalar_ctx_t *ctx = context;
	omp_set_num_threads(ctx->P);
	
	for (int i = 0; i < ctx->n; ++i) {
		fprintf(data, "%d ", ctx->data[i]);
	}
	
	fprintf(data, "\n");
	
	double start = omp_get_wtime();
	parallel_merge_sort(ctx->data, 0, ctx->n - 1, ctx->sorted, 0, ctx->m);
	double end = omp_get_wtime();
	double merge_sort_elapsed = end - start;
	
	start = omp_get_wtime();
    	qsort(ctx->data, ctx->n, sizeof(int), cmpfunc);
	end = omp_get_wtime();
	double quicksort_elapsed = end - start;
	
	fprintf(stats, "%f %f %d %d %d \\\n", quicksort_elapsed, merge_sort_elapsed, ctx->n, ctx->m, ctx->P);
	for (int i = 0; i < ctx->n; ++i) {
		fprintf(data, "%d ", ctx->sorted[i]);
	}
}

int main (int argc, char **argv) {
	if( argc == 4 ) {
		int n = atoi(argv[1]);
		int m = atoi(argv[2]);
		int P = atoi(argv[3]);
		
		scalar_ctx_t ctx = {
			.n = n,
			.m = m,
			.P = P,
		};
		
		ctx.data = calloc(ctx.n, sizeof(int));
		assert(ctx.data);
		
		ctx.sorted = calloc(ctx.n, sizeof(int));
		assert(ctx.sorted);
		
		srand(time(NULL));
		for (int i = 0; i < ctx.n; ++i) {
			ctx.data[i] = rand()%10000;
			ctx.sorted[i] = ctx.data[i];
		}
		
		FILE *stats = fopen("stats.txt", "w");
		FILE *data = fopen("data.txt", "w");
		
		if (stats != NULL && data != NULL) {
			ctx.P = 1;
			merge_sort(&ctx, stats, data);
			free(ctx.sorted);
			ctx.sorted = calloc(ctx.n, sizeof(int));
			assert(ctx.sorted);
			srand(time(NULL));
			for (int i = 0; i < ctx.n; ++i) {
				ctx.sorted[i] = ctx.data[i];
			}

			ctx.P = 2;
			merge_sort(&ctx, stats, data);
			free(ctx.sorted);
			ctx.sorted = calloc(ctx.n, sizeof(int));
			assert(ctx.sorted);
			srand(time(NULL));
			for (int i = 0; i < ctx.n; ++i) {
				ctx.sorted[i] = ctx.data[i];
			}

			ctx.P = 4;
			merge_sort(&ctx, stats, data);
			free(ctx.sorted);
			ctx.sorted = calloc(ctx.n, sizeof(int));
			assert(ctx.sorted);
			srand(time(NULL));
			for (int i = 0; i < ctx.n; ++i) {
				ctx.sorted[i] = ctx.data[i];
			}

			ctx.P = 8;
			merge_sort(&ctx, stats, data);
			free(ctx.sorted);
			ctx.sorted = calloc(ctx.n, sizeof(int));
			assert(ctx.sorted);
			srand(time(NULL));
			for (int i = 0; i < ctx.n; ++i) {
				ctx.sorted[i] = ctx.data[i];
			}

			ctx.P = 16;
			merge_sort(&ctx, stats, data);
		}
		
		fclose(stats);
		fclose(data);
		
		free(ctx.data);
		free(ctx.sorted);
	}
	return 0;
}
