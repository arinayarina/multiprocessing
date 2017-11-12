#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <time.h>
#include <string.h>

int n; // количество чисел в сортируемом массиве
int P; // количество потоков
int m; // максимальный размер чанка

int cmpfunc (const void * a, const void * b) {
    return ( *(int*)a - *(int*)b );
}

typedef struct chunks_data_t {
    int start;        // начало сортируемой части
    int chunk_number; // количество сортируемых чанков
    int* chunks;      // указатель на массив чанков
} chunks_data_t;

typedef struct pthread_data_t {
    int left1;
    int right1;
    int left2;
    int right2;
    int* pointer;
} pthread_data_t;

void simple_merge (int* input, int left1, int right1, int left2, int right2) {
    int i = 0;
    int j = 0;
    int n1 = right1 - left1 + 1;
    int n2 = right2 - left2 + 1;
    int* temp1 = malloc(n1 * sizeof(int));
    int* temp2 = malloc(n2 * sizeof(int));
    
    memcpy(temp1, &input[left1], n1 * sizeof(int));
    memcpy(temp2, &input[left2], n2 * sizeof(int));
    
    while((i < n1) && (j < n2)) {
        if(temp1[i] < temp2[j]) {
            input[left1 + i + j] = temp1[i];
            i++;
        } else {
            input[left1 + i + j] = temp2[j];
            j++;
        }
    }
	
    while(i < n1) {
        input[left1 + i + j] = temp1[i];
        i++;
    }
	
    while(j < n2) {
        input[left1 + i + j] = temp2[j];
        j++;
    }
    
    free(temp1);
    free(temp2);
}

void* thread_sort_chunks (void* chunks_data_) {
    chunks_data_t* chunks_data = (chunks_data_t*) chunks_data_;    
    int cur_index = chunks_data->start;
    for (int i = 0; i < chunks_data->chunk_number; i++) {
        qsort(&chunks_data->chunks[cur_index], m, sizeof(int), cmpfunc);
        cur_index += m;
    }
    return NULL;
}

void* thread_simple_merge (void* data_) {
    pthread_data_t* data = (pthread_data_t*) data_;
    simple_merge(data->pointer, data->left1, data->right1, data->left2, data->right2);
    return NULL;
}

void sort_chunks (int* input, int n) {
    pthread_t* threads = (pthread_t*) malloc(P * sizeof(pthread_t));
    assert(threads);
    chunks_data_t* thread_data = malloc(P * sizeof(chunks_data_t));
    assert(thread_data);
    
    int chunks_amount = n / m;
    int chunk_number = chunks_amount / P;
    int cur_index = 0;
    
    for (int i = 0; i < P; i++) {
        thread_data[i].start = cur_index;
        thread_data[i].chunk_number = chunk_number;
        thread_data[i].chunks = input;
        pthread_create(&(threads[i]), NULL, thread_sort_chunks, &thread_data[i]);
        cur_index += m * chunk_number;
    }
    
    for (int i = 0; i < P; i++) {
        pthread_join(threads[i], NULL);
    }
    
    qsort(&input[cur_index], n - cur_index, sizeof(int), cmpfunc);
    
    free(threads);
    free(thread_data);
}

void p_merge_sort (int* input, int n) {    
    sort_chunks(input, n);
    
    pthread_t* threads = malloc(P * sizeof(pthread_t));
    assert(threads);
    pthread_data_t* thread_data = malloc(P * sizeof(pthread_data_t));
    assert(thread_data);
    
    for (int cur_size = m; cur_size < n; cur_size *= 2) {
        int cur_index = 0;
        int num_threads = P;
        while (cur_index + cur_size + 1 < n) {
            for (int i = 0; i < P; i++) {
                int left1 = cur_index;
                int right1 = cur_index + cur_size - 1;
                if (right1 >= n) {
                    num_threads = i;
                    break;
                }
                int left2 = right1 + 1;
                int right2 = left2 + cur_size - 1;
                if (right2 > n - 1) {
                    right2 = n - 1;
                }
                cur_index = right2 + 1;
                
                thread_data[i].left1 = left1;
                thread_data[i].right1 = right1;
                thread_data[i].left2 = left2;
                thread_data[i].right2 = right2;
                thread_data[i].pointer = input;
                
                pthread_create(&(threads[i]), NULL, thread_simple_merge, &thread_data[i]);
            }
            
            for (int i = 0; i < num_threads; i++) {
                pthread_join(threads[i], NULL);
            }
        }
    }
    
    free(threads);
    free(thread_data);
}

void merge_sort (FILE *stats, FILE *data) {
    int *input = calloc(n, sizeof(int));
    assert(input);
	
    int *sorted = calloc(n, sizeof(int));
    assert(sorted);
	
    srand(time(NULL));
    for (int i = 0; i < n; ++i) {
        input[i] = rand()%10000;
        sorted[i] = input[i];
    }
	
    for (int i = 0; i < n; ++i) {
        fprintf(data, "%d ", input[i]);
    }
	
    fprintf(data, "\n");
	
    struct timespec start, finish;
    double merge_sort_elapsed;
    clock_gettime(CLOCK_MONOTONIC, &start);
    p_merge_sort(sorted, n);
    clock_gettime(CLOCK_MONOTONIC, &finish);
    merge_sort_elapsed = (finish.tv_sec - start.tv_sec);
    merge_sort_elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
    
    for (int i = 0; i < n; i++) {
        fprintf(data, "%d ", sorted[i]);
    }
    
    fprintf(data, "\n\n");
    fclose(data);
    
    double quicksort_elapsed;
    clock_gettime(CLOCK_MONOTONIC, &start);
    qsort(&input[0], n, sizeof(int), cmpfunc);
    clock_gettime(CLOCK_MONOTONIC, &finish);
    quicksort_elapsed = (finish.tv_sec - start.tv_sec);
    quicksort_elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;

    stats = fopen("stats.txt", "a");
    fseek(stats, 0, SEEK_END);
    fprintf(stats, "%f %f %d %d %d\n", merge_sort_elapsed, quicksort_elapsed, n, m, P);
    
    free(input);
    free(sorted);
}

int main (int argc, char **argv) {
	if( argc == 4 ) {
		n = atoi(argv[1]);
		m = atoi(argv[2]);
		P = atoi(argv[3]);
		FILE *stats = fopen("stats.txt", "w");
		FILE *data = fopen("data.txt", "w");
		
		if (stats != NULL && data != NULL) {
			merge_sort(stats, data);
		}
		
		fclose(stats);
		fclose(data);
	}
	return 0;
}
