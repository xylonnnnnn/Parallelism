#include <omp.h>
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

enum class Variant {
    ManyParallelFor = 1,
    SingleParallelRegion = 2
};

static double residual_value(double sum_x, std::size_t n) {
    return std::abs(sum_x - static_cast<double>(n));
}

static double solve_variant(std::size_t n, double eps, int max_iters, int threads, Variant variant, int& iters_done, double& final_residual) {
    omp_set_num_threads(threads);

    std::vector<double> x(n, 0.0);
    std::vector<double> x_next(n, 0.0);

    const double tau = 0.9 / (static_cast<double>(n) + 1.0);
    const double start = omp_get_wtime();

    iters_done = 0;
    final_residual = std::numeric_limits<double>::infinity();

    if (variant == Variant::ManyParallelFor) {
        for (int iter = 0; iter < max_iters; ++iter) {
            double sum_x = 0.0;

            #pragma omp parallel for reduction(+:sum_x) schedule(static)
            for (std::size_t i = 0; i < n; ++i) {
                sum_x += x[i];
            }

            final_residual = residual_value(sum_x, n);
            if (final_residual < eps) {
                iters_done = iter;
                break;
            }

            #pragma omp parallel for schedule(static)
            for (std::size_t i = 0; i < n; ++i) {
                const double ax_i = sum_x + x[i];
                x_next[i] = x[i] - tau * (ax_i - (static_cast<double>(n) + 1.0));
            }

            #pragma omp parallel for schedule(static)
            for (std::size_t i = 0; i < n; ++i) {
                x[i] = x_next[i];
            }

            iters_done = iter + 1;
        }
    } else {
        bool stop = false;
        double sum_x = 0.0;

        #pragma omp parallel shared(stop, sum_x, iters_done, final_residual)
        {
            for (int iter = 0; iter < max_iters; ++iter) {
                #pragma omp single
                {
                    sum_x = 0.0;
                }

                #pragma omp for reduction(+:sum_x) schedule(static)
                for (std::size_t i = 0; i < n; ++i) {
                    sum_x += x[i];
                }

                #pragma omp single
                {
                    final_residual = residual_value(sum_x, n);
                    stop = final_residual < eps;
                    if (stop) {
                        iters_done = iter;
                    }
                }

                #pragma omp barrier
                if (stop) {
                    break;
                }

                #pragma omp for schedule(static)
                for (std::size_t i = 0; i < n; ++i) {
                    const double ax_i = sum_x + x[i];
                    x_next[i] = x[i] - tau * (ax_i - (static_cast<double>(n) + 1.0));
                }

                #pragma omp for schedule(static)
                for (std::size_t i = 0; i < n; ++i) {
                    x[i] = x_next[i];
                }

                #pragma omp single
                {
                    iters_done = iter + 1;
                }

                #pragma omp barrier
            }
        }
    }

    return omp_get_wtime() - start;
}

int main() {
    const std::size_t n = 10000000;
    const double eps = 1e-5;
    const int max_iters = 100000;
    const int threads = 8;

    int iters_1 = 0;
    int iters_2 = 0;
    double residual_1 = 0.0;
    double residual_2 = 0.0;

    const double time_1 = solve_variant(n, eps, max_iters, threads, Variant::ManyParallelFor, iters_1, residual_1);

    const double time_2 = solve_variant(n, eps, max_iters, threads, Variant::SingleParallelRegion, iters_2, residual_2);

    std::cout << "Problem size n = " << n << '\n';
    std::cout << "Threads = " << threads << '\n';
    std::cout << "Eps = " << eps << '\n';
    std::cout << "Max iterations = " << max_iters << "\n\n";

    std::cout << "Variant 1: parallel_for_each_loop\n";
    std::cout << "Time      = " << time_1 << " sec\n";
    std::cout << "Iterations= " << iters_1 << '\n';
    std::cout << "Residual  = " << residual_1 << "\n\n";

    std::cout << "Variant 2: single_parallel_region\n";
    std::cout << "Time      = " << time_2 << " sec\n";
    std::cout << "Iterations= " << iters_2 << '\n';
    std::cout << "Residual  = " << residual_2 << '\n';

    return 0;
}
