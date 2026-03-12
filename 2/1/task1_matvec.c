#include <errno.h>
#include <inttypes.h>
#include <omp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static double wtime(void) {
    return omp_get_wtime();
}

static void *xmalloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "malloc(%zu) failed: %s\n", size, strerror(errno));
        exit(EXIT_FAILURE);
    }
    return ptr;
}

static void init_arrays_serial(double *a, double *b, int m, int n) {
    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < n; ++j) {
            a[(size_t)i * (size_t)n + (size_t)j] = 1.0 / (1.0 + i + j);
        }
    }
    for (int j = 0; j < n; ++j) {
        b[j] = 1.0 + (double)(j % 1000) * 1e-3;
    }
}

static void init_arrays_omp(double *a, double *b, int m, int n) {
#pragma omp parallel for schedule(static)
    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < n; ++j) {
            a[(size_t)i * (size_t)n + (size_t)j] = 1.0 / (1.0 + i + j);
        }
    }
#pragma omp parallel for schedule(static)
    for (int j = 0; j < n; ++j) {
        b[j] = 1.0 + (double)(j % 1000) * 1e-3;
    }
}

static void matvec_serial(const double *a, const double *b, double *c, int m, int n) {
    for (int i = 0; i < m; ++i) {
        double sum = 0.0;
        for (int j = 0; j < n; ++j) {
            sum += a[(size_t)i * (size_t)n + (size_t)j] * b[j];
        }
        c[i] = sum;
    }
}

static void matvec_omp(const double *a, const double *b, double *c, int m, int n) {
#pragma omp parallel
    {
        int nthreads = omp_get_num_threads();
        int tid = omp_get_thread_num();
        int items_per_thread = m / nthreads;
        int lb = tid * items_per_thread;
        int ub = (tid == nthreads - 1) ? (m - 1) : (lb + items_per_thread - 1);

        for (int i = lb; i <= ub; ++i) {
            double sum = 0.0;
            for (int j = 0; j < n; ++j) {
                sum += a[(size_t)i * (size_t)n + (size_t)j] * b[j];
            }
            c[i] = sum;
        }
    }
}

static double checksum(const double *x, int n) {
    double sum = 0.0;
    for (int i = 0; i < n; ++i) {
        sum += x[i];
    }
    return sum;
}

static void usage(const char *prog) {
    fprintf(stderr,
            "Usage: %s <m> <n> <threads> <mode>\n"
            "  mode: serial | omp\n",
            prog);
}

int main(int argc, char **argv) {
    if (argc != 5) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    int m = atoi(argv[1]);
    int n = atoi(argv[2]);
    int threads = atoi(argv[3]);
    const char *mode = argv[4];

    if (m <= 0 || n <= 0 || threads <= 0) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    omp_set_dynamic(0);
    omp_set_num_threads(threads);

    uint64_t elems = (uint64_t)m * (uint64_t)n;
    uint64_t mem_bytes = (elems + (uint64_t)m + (uint64_t)n) * sizeof(double);

    double *a = xmalloc((size_t)elems * sizeof(*a));
    double *b = xmalloc((size_t)n * sizeof(*b));
    double *c = xmalloc((size_t)m * sizeof(*c));

    double t0 = wtime();
    if (strcmp(mode, "serial") == 0) {
        init_arrays_serial(a, b, m, n);
    } else if (strcmp(mode, "omp") == 0) {
        init_arrays_omp(a, b, m, n);
    } else {
        usage(argv[0]);
        free(a);
        free(b);
        free(c);
        return EXIT_FAILURE;
    }
    double init_time = wtime() - t0;

    t0 = wtime();
    if (strcmp(mode, "serial") == 0) {
        matvec_serial(a, b, c, m, n);
    } else {
        matvec_omp(a, b, c, m, n);
    }
    double compute_time = wtime() - t0;

    printf("task=1 mode=%s threads=%d m=%d n=%d memory_mib=%" PRIu64 " init_sec=%.6f compute_sec=%.6f total_sec=%.6f checksum=%.12e\n",
           mode, threads, m, n, mem_bytes >> 20, init_time, compute_time, init_time + compute_time, checksum(c, m));

    free(a);
    free(b);
    free(c);
    return EXIT_SUCCESS;
}
