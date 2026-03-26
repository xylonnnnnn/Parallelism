#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <omp.h>

struct Result {
    int iterations;
    double norm;
    double error;
    double checksum;
    double elapsed;
};

static void init_problem(std::vector<double> &b, std::vector<double> &x, int n) {
    std::fill(b.begin(), b.end(), static_cast<double>(n + 1));
    std::fill(x.begin(), x.end(), 0.0);
}

static double tau_for_problem(int n) {
    return 2.0 / (static_cast<double>(n) + 2.0);
}

static Result solve_variant1(int n, double eps, int max_iters, omp_sched_t sched, int chunk) {
    std::vector<double> x(n, 0.0), x_new(n, 0.0), b(n);
    init_problem(b, x, n);
    omp_set_schedule(sched, chunk);
    const double tau = tau_for_problem(n);

    double t0 = omp_get_wtime();
    int it = 0;
    double norm = 0.0;

    for (it = 1; it <= max_iters; ++it) {
        double sum_x = 0.0;
        #pragma omp parallel for reduction(+:sum_x) schedule(runtime)
        for (int i = 0; i < n; ++i) {
            sum_x += x[i];
        }

        #pragma omp parallel for schedule(runtime)
        for (int i = 0; i < n; ++i) {
            double ax_i = sum_x + x[i];
            x_new[i] = x[i] - tau * (ax_i - b[i]);
        }

        norm = 0.0;
        #pragma omp parallel for reduction(max:norm) schedule(runtime)
        for (int i = 0; i < n; ++i) {
            double diff = std::fabs(x_new[i] - x[i]);
            norm = std::max(norm, diff);
        }

        #pragma omp parallel for schedule(runtime)
        for (int i = 0; i < n; ++i) {
            x[i] = x_new[i];
        }

        if (norm < eps) {
            break;
        }
    }

    double elapsed = omp_get_wtime() - t0;
    double error = 0.0;
    double checksum = 0.0;
    for (int i = 0; i < n; ++i) {
        error = std::max(error, std::fabs(x[i] - 1.0));
        checksum += x[i];
    }

    return {it, norm, error, checksum, elapsed};
}

static Result solve_variant2(int n, double eps, int max_iters, omp_sched_t sched, int chunk) {
    std::vector<double> x(n, 0.0), x_new(n, 0.0), b(n);
    init_problem(b, x, n);
    omp_set_schedule(sched, chunk);
    const double tau = tau_for_problem(n);

    double t0 = omp_get_wtime();
    int iterations = max_iters;
    double norm = 0.0;
    double sum_x = 0.0;
    int stop = 0;

    #pragma omp parallel shared(x, x_new, b, norm, sum_x, stop, iterations)
    {
        for (int it = 1; it <= max_iters; ++it) {
            #pragma omp single
            {
                sum_x = 0.0;
                norm = 0.0;
            }

            #pragma omp for reduction(+:sum_x) schedule(runtime)
            for (int i = 0; i < n; ++i) {
                sum_x += x[i];
            }

            #pragma omp for schedule(runtime)
            for (int i = 0; i < n; ++i) {
                double ax_i = sum_x + x[i];
                x_new[i] = x[i] - tau * (ax_i - b[i]);
            }

            #pragma omp for reduction(max:norm) schedule(runtime)
            for (int i = 0; i < n; ++i) {
                double diff = std::fabs(x_new[i] - x[i]);
                norm = std::max(norm, diff);
            }

            #pragma omp for schedule(runtime)
            for (int i = 0; i < n; ++i) {
                x[i] = x_new[i];
            }

            #pragma omp single
            {
                if (norm < eps) {
                    stop = 1;
                    iterations = it;
                }
            }

            #pragma omp barrier
            if (stop) {
                break;
            }
        }
    }

    double elapsed = omp_get_wtime() - t0;
    double error = 0.0;
    double checksum = 0.0;
    for (int i = 0; i < n; ++i) {
        error = std::max(error, std::fabs(x[i] - 1.0));
        checksum += x[i];
    }

    return {iterations, norm, error, checksum, elapsed};
}

static omp_sched_t parse_schedule(const std::string &name) {
    if (name == "static") return omp_sched_static;
    if (name == "dynamic") return omp_sched_dynamic;
    if (name == "guided") return omp_sched_guided;
    if (name == "auto") return omp_sched_auto;
    throw std::runtime_error("Unknown schedule: " + name);
}

int main(int argc, char **argv) {
    if (argc != 7) {
        std::cerr << "Usage: " << argv[0]
                  << " <n> <threads> <variant> <eps> <max_iters> <schedule[:chunk]>\n"
                  << "  variant: v1 | v2\n";
        return 1;
    }

    int n = std::atoi(argv[1]);
    int threads = std::atoi(argv[2]);
    std::string variant = argv[3];
    double eps = std::atof(argv[4]);
    int max_iters = std::atoi(argv[5]);
    std::string sched_arg = argv[6];

    if (n <= 0 || threads <= 0 || eps <= 0.0 || max_iters <= 0) {
        std::cerr << "Invalid arguments\n";
        return 1;
    }

    std::string sched_name = sched_arg;
    int chunk = 0;
    auto pos = sched_arg.find(':');
    if (pos != std::string::npos) {
        sched_name = sched_arg.substr(0, pos);
        chunk = std::atoi(sched_arg.substr(pos + 1).c_str());
    }

    omp_set_dynamic(0);
    omp_set_num_threads(threads);

    Result res{};
    try {
        omp_sched_t sched = parse_schedule(sched_name);
        if (variant == "v1") {
            res = solve_variant1(n, eps, max_iters, sched, chunk);
        } else if (variant == "v2") {
            res = solve_variant2(n, eps, max_iters, sched, chunk);
        } else {
            std::cerr << "Unknown variant: " << variant << '\n';
            return 1;
        }
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }

    std::cout << "task=3 variant=" << variant
              << " threads=" << threads
              << " n=" << n
              << " eps=" << eps
              << " max_iters=" << max_iters
              << " iterations=" << res.iterations
              << " norm=" << res.norm
              << " error=" << res.error
              << " checksum=" << res.checksum
              << " time_sec=" << res.elapsed
              << "\n";
    return 0;
}
