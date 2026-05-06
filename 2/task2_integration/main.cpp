#include <omp.h>
#include <cmath>
#include <iostream>

double f(double x) {
    return 4.0 / (1.0 + x * x);
}

double integrate_serial(long long nsteps) {
    const double step = 1.0 / static_cast<double>(nsteps);
    double sum = 0.0;

    for (long long i = 0; i < nsteps; ++i) {
        const double x = (static_cast<double>(i) + 0.5) * step;
        sum += f(x);
    }

    return sum * step;
}

double integrate_omp_atomic(long long nsteps, int threads) {
    const double step = 1.0 / static_cast<double>(nsteps);
    double global_sum = 0.0;

    omp_set_num_threads(threads);

    #pragma omp parallel
    {
        double local_sum = 0.0;

        #pragma omp for schedule(static)
        for (long long i = 0; i < nsteps; ++i) {
            const double x = (static_cast<double>(i) + 0.5) * step;
            local_sum += f(x);
        }

        #pragma omp atomic
        global_sum += local_sum;
    }

    return global_sum * step;
}

int main() {
    const long long nsteps = 40000000LL;
    const int threads = 8;

    const double t0 = omp_get_wtime();
    const double pi_parallel = integrate_omp_atomic(nsteps, threads);
    const double t1 = omp_get_wtime();

    const double pi_serial = integrate_serial(nsteps);
    const double exact_pi = std::acos(-1.0);

    std::cout << "nsteps: " << nsteps << '\n';
    std::cout << "threads: " << threads << '\n';
    std::cout << "parallel pi = " << pi_parallel << '\n';
    std::cout << "serial pi   = " << pi_serial << '\n';
    std::cout << "abs error   = " << std::abs(pi_parallel - exact_pi) << '\n';
    std::cout << "time        = " << (t1 - t0) << " sec\n";

    return 0;
}
