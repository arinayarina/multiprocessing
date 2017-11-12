#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>

typedef struct scalar_ctx_t {
	int l;
	int a;
	int b;
	int N;
	int* d;
} scalar_ctx_t;

void func_io (void *context) {
	scalar_ctx_t *ctx = context;
	int seed = time(NULL);
	struct timespec start, finish;
	int rank;
	
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	if (!rank) {
		int comm_size;
		int *seeds;

		MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
		if (comm_size != ctx->a * ctx->b) {
			MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
		}

		seeds = malloc(comm_size * sizeof(int));
		seed = time(NULL);
		for (int i = 0; i < comm_size; ++i) {
			seeds[i] = seed + i;
		}

		MPI_Scatter(seeds, 1, MPI_INT, &seed, 1, MPI_INT, 0, MPI_COMM_WORLD);
		clock_gettime(CLOCK_MONOTONIC, &start);
		free(seeds);
	} else {
		MPI_Scatter(NULL, 1, MPI_INT, &seed, 1, MPI_INT, 0, MPI_COMM_WORLD);
	}
	
	srand(seed);
	int ab = ctx->a * ctx->b;
	for (int i = 0; i < ctx->N; i++) {
		int x = rand() % ctx->l;
		int y = rand() % ctx->l;
		int r = rand() % ab;
		ctx->d[(y * ctx->l + x) * ab + r] += 1;
	}
	
	MPI_File data;
	MPI_Datatype contiguous, view;
	
	MPI_File_open(MPI_COMM_WORLD, "data.bin", MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &data);
	MPI_File_set_size(data, 0);
	
	MPI_Type_contiguous(ctx->l * ab, MPI_INT, &contiguous);
	MPI_Type_create_resized(contiguous, 0, ctx->a * ctx->l * ab * sizeof(int), &view);
	MPI_Type_commit(&view);
	MPI_File_set_view(data, ((rank / ctx->a) * ctx->a * ctx->l + rank % ctx->a) * 
			ctx->l * ab * sizeof(int), MPI_INT, view, "native", MPI_INFO_NULL);
	MPI_File_write_all(data, ctx->d, ctx->l * ctx->l * ab, MPI_INT, MPI_STATUS_IGNORE);

	MPI_File_close(&data);
	free(ctx->d);

	if (!rank) {
		FILE *stats = fopen("stats.txt", "w");
		clock_gettime(CLOCK_MONOTONIC, &finish);
		double elapsed_time = finish.tv_sec - start.tv_sec;
		elapsed_time += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
		fprintf(stats, "%d %d %d %d %fs\n", ctx->l, ctx->a, ctx->b, ctx->N, elapsed_time);
		fclose(stats);
	}
}

int main(int argc, char **argv) {
	MPI_Init(&argc, &argv);
	if (argc == 5) {
		scalar_ctx_t ctx = {
			.l = atoi(argv[1]),
			.a = atoi(argv[2]),
			.b = atoi(argv[3]),
			.N = atoi(argv[4]),
			.d = 0
		};
		ctx.d = calloc(ctx.l * ctx.l * ctx.a * ctx.b, sizeof(int));
		func_io(&ctx);
	}
	MPI_Finalize();
	return 0;
}
