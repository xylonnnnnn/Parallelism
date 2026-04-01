
#include <omp.h>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

struct Config {
    long long nsteps = 40000000LL;
    std::vector<int> threads{1, 2, 4, 7, 8, 16, 20, 40};
    int repeats = 5;
    fs::path output_csv = "tables/integration_summary.csv";
};

static void ensure_parent_dir(const fs::path& path) {
    if (path.has_parent_path()) {
        fs::create_directories(path.parent_path());
    }
}

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

double benchmark_parallel(long long nsteps, int threads, int repeats, double& result) {
    double best_time = std::numeric_limits<double>::max();
    for (int rep = 0; rep < repeats; ++rep) {
        const double t0 = omp_get_wtime();
        result = integrate_omp_atomic(nsteps, threads);
        const double t1 = omp_get_wtime();
        best_time = std::min(best_time, t1 - t0);
    }
    return best_time;
}

double benchmark_serial(long long nsteps, int repeats, double& result) {
    double best_time = std::numeric_limits<double>::max();
    for (int rep = 0; rep < repeats; ++rep) {
        const double t0 = omp_get_wtime();
        result = integrate_serial(nsteps);
        const double t1 = omp_get_wtime();
        best_time = std::min(best_time, t1 - t0);
    }
    return best_time;
}

int main(int argc, char** argv) {
    Config cfg;

    try {
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--nsteps" && i + 1 < argc) {
                cfg.nsteps = std::stoll(argv[++i]);
            } else if (arg == "--repeats" && i + 1 < argc) {
                cfg.repeats = std::stoi(argv[++i]);
            } else if (arg == "--output" && i + 1 < argc) {
                cfg.output_csv = argv[++i];
            } else if (arg == "--help") {
                std::cout
                    << "OpenMP integration benchmark\n"
                    << "Options:\n"
                    << "  --nsteps N        Integration steps (default: 40000000)\n"
                    << "  --repeats N       Repeats per configuration (default: 5)\n"
                    << "  --output FILE     Summary CSV (default: tables/integration_summary.csv)\n";
                return 0;
            } else {
                throw std::runtime_error("Unknown argument: " + arg);
            }
        }

        ensure_parent_dir(cfg.output_csv);
        std::ofstream out(cfg.output_csv);
        if (!out) {
            throw std::runtime_error("Cannot open output file");
        }

        out << "threads,time_sec,speedup,efficiency,pi_estimate,abs_error\n";
        out << std::fixed << std::setprecision(9);

        double serial_value = 0.0;
        const double t1 = benchmark_serial(cfg.nsteps, cfg.repeats, serial_value);

        for (int threads : cfg.threads) {
            double pi_value = 0.0;
            const double tp = benchmark_parallel(cfg.nsteps, threads, cfg.repeats, pi_value);
            const double sp = t1 / tp;
            const double ep = sp / static_cast<double>(threads);
            const double err = std::abs(pi_value - std::acos(-1.0));

            out << threads << ',' << tp << ',' << sp << ',' << ep << ','
                << pi_value << ',' << err << '\n';

            std::cout << "threads=" << threads
                      << " time=" << tp
                      << " speedup=" << sp
                      << " pi=" << pi_value << '\n';
        }

        std::cout << "serial_time=" << t1 << ", serial_pi=" << serial_value << '\n';
        std::cout << "Saved summary to: " << cfg.output_csv << '\n';
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "ERROR: " << ex.what() << '\n';
        return 1;
    }
}
