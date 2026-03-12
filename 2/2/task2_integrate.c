#include <math.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static double func(double x) {
    return exp(-x * x);
}

static double integrate_serial(double a, double b, int n) {
    double h = (b - a) / n;
    double sum = 0.0;
    for (int i = 0; i < n; ++i) {
        sum += func(a + h * (i + 0.5));
    }
    return sum * h;
}

static double integrate_omp_atomic(double a, double b, int n) {
    double h = (b - a) / n;
    double sum = 0.0;
#pragma omp parallel
    {
        int nthreads = omp_get_num_threads();
        int tid = omp_get_thread_num();
        int items_per_thread = n / nthreads;
        int lb = tid * items_per_thread;
        int ub = (tid == nthreads - 1) ? (n - 1) : (lb + items_per_thread - 1);

        for (int i = lb; i <= ub; ++i) {
            double v = func(a + h * (i + 0.5));
#pragma omp atomic
            sum += v;
        }
    }
    return sum * h;
}

static double integrate_omp_atomic_local(double a, double b, int n) {
    double h = (b - a) / n;
    double sum = 0.0;
#pragma omp parallel
    {
        int nthreads = omp_get_num_threads();
        int tid = omp_get_thread_num();
        int items_per_thread = n / nthreads;
        int lb = tid * items_per_thread;
        int ub = (tid == nthreads - 1) ? (n - 1) : (lb + items_per_thread - 1);
        double sumloc = 0.0;

        for (int i = lb; i <= ub; ++i) {
            sumloc += func(a + h * (i + 0.5));
        }
#pragma omp atomic
        sum += sumloc;
    }
    return sum * h;
}

static void usage(const char *prog) {
    fprintf(stderr,
            "Usage: %s <threads> <nsteps> <mode>\n"
            "  mode: serial | atomic | atomic_local\n",
            prog);
}

int main(int argc, char **argv) {
    const double a = -4.0;
    const double b = 4.0;

    if (argc != 4) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    int threads = atoi(argv[1]);
    int nsteps = atoi(argv[2]);
    const char *mode = argv[3];

    if (threads <= 0 || nsteps <= 0) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    omp_set_dynamic(0);
    omp_set_num_threads(threads);

    double t0 = omp_get_wtime();
    double result = 0.0;

    if (strcmp(mode, "serial") == 0) {
        result = integrate_serial(a, b, nsteps);
    } else if (strcmp(mode, "atomic") == 0) {
        result = integrate_omp_atomic(a, b, nsteps);
    } else if (strcmp(mode, "atomic_local") == 0) {
        result = integrate_omp_atomic_local(a, b, nsteps);
    } else {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    double elapsed = omp_get_wtime() - t0;
    printf("task=2 mode=%s threads=%d nsteps=%d result=%.15f time_sec=%.6f\n",
           mode, threads, nsteps, result, elapsed);
    return EXIT_SUCCESS;
}
