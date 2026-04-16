#include <omp.h>
#include <cstdint>
#include <iostream>
#include <vector>

static double multiply_matrix_vector(std::int64_t n, int threads, double& checksum) {
    const std::size_t matrix_elems =
        static_cast<std::size_t>(n) * static_cast<std::size_t>(n);

    std::vector<double> A(matrix_elems);
    std::vector<double> x(static_cast<std::size_t>(n));
    std::vector<double> y(static_cast<std::size_t>(n), 0.0);

    omp_set_num_threads(threads);

    #pragma omp parallel for schedule(static)
    for (std::int64_t i = 0; i < n; ++i) {
        x[static_cast<std::size_t>(i)] = 1.0 + static_cast<double>(i % 13) * 1e-3;
    }

    #pragma omp parallel for schedule(static)
    for (std::int64_t i = 0; i < n; ++i) {
        const std::size_t row =
            static_cast<std::size_t>(i) * static_cast<std::size_t>(n);

        for (std::int64_t j = 0; j < n; ++j) {
            A[row + static_cast<std::size_t>(j)] = (i == j) ? 2.0 : 1.0;
        }
    }

    const double t0 = omp_get_wtime();

    #pragma omp parallel for schedule(static)
    for (std::int64_t i = 0; i < n; ++i) {
        const std::size_t row =
            static_cast<std::size_t>(i) * static_cast<std::size_t>(n);

        double sum = 0.0;

        #pragma omp simd reduction(+:sum)
        for (std::int64_t j = 0; j < n; ++j) {
            sum += A[row + static_cast<std::size_t>(j)] *
                   x[static_cast<std::size_t>(j)];
        }

        y[static_cast<std::size_t>(i)] = sum;
    }

    const double t1 = omp_get_wtime();

    checksum = 0.0;

    #pragma omp parallel for reduction(+:checksum) schedule(static)
    for (std::int64_t i = 0; i < n; ++i) {
        checksum += y[static_cast<std::size_t>(i)];
    }

    return t1 - t0;
}

int main() {
    const std::int64_t n = 20000;
    const int threads = 8;

    double checksum = 0.0;
    const double elapsed = multiply_matrix_vector(n, threads, checksum);

    std::cout << "Matrix size: " << n << " x " << n << '\n';
    std::cout << "Threads: " << threads << '\n';
    std::cout << "Elapsed time: " << elapsed << " sec\n";
    std::cout << "Checksum: " << checksum << '\n';

    return 0;
}
