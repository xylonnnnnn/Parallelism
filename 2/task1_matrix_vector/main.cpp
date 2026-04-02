#include <omp.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

struct BenchmarkConfig {
    std::vector<std::int64_t> sizes{20000, 40000};
    std::vector<int> threads{1, 2, 4, 7, 8, 16, 20, 40};
    int repeats = 3;
    fs::path output_csv = "tables/matrix_vector_results.csv";
};

static void ensure_parent_dir(const fs::path& path) {
    if (path.has_parent_path()) {
        fs::create_directories(path.parent_path());
    }
}

static double wall_seconds() {
    return omp_get_wtime();
}

static double initialize_and_run(std::int64_t n, int threads, double& checksum) {
    const std::size_t matrix_elems = static_cast<std::size_t>(n) * static_cast<std::size_t>(n);
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
        const std::size_t row = static_cast<std::size_t>(i) * static_cast<std::size_t>(n);
        for (std::int64_t j = 0; j < n; ++j) {
            A[row + static_cast<std::size_t>(j)] = (i == j) ? 2.0 : 1.0;
        }
    }

    const double t0 = wall_seconds();

    #pragma omp parallel for schedule(static)
    for (std::int64_t i = 0; i < n; ++i) {
        const std::size_t row = static_cast<std::size_t>(i) * static_cast<std::size_t>(n);
        double sum = 0.0;
        #pragma omp simd reduction(+:sum)
        for (std::int64_t j = 0; j < n; ++j) {
            sum += A[row + static_cast<std::size_t>(j)] * x[static_cast<std::size_t>(j)];
        }
        y[static_cast<std::size_t>(i)] = sum;
    }

    const double t1 = wall_seconds();

    checksum = 0.0;
    #pragma omp parallel for reduction(+:checksum) schedule(static)
    for (std::int64_t i = 0; i < n; ++i) {
        checksum += y[static_cast<std::size_t>(i)];
    }

    return t1 - t0;
}

int main(int argc, char** argv) {
    BenchmarkConfig config;

    try {
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--repeats" && i + 1 < argc) {
                config.repeats = std::stoi(argv[++i]);
            } else if (arg == "--output" && i + 1 < argc) {
                config.output_csv = argv[++i];
            } else {
                throw std::runtime_error("Unknown argument: " + arg);
            }
        }

        ensure_parent_dir(config.output_csv);
        std::ofstream out(config.output_csv);
        if (!out) {
            throw std::runtime_error("Cannot open output file: " + config.output_csv.string());
        }

        out << "matrix_size,threads,repeat,time_sec,checksum,speedup\n";
        out << std::fixed << std::setprecision(6);

        for (std::int64_t n : config.sizes) {
            for (int threads : config.threads) {
                for (int repeat = 1; repeat <= config.repeats; ++repeat) {
                    double checksum = 0.0;
                    const double elapsed = initialize_and_run(n, threads, checksum);

                    std::cout << "N=" << n
                              << ", threads=" << threads
                              << ", repeat=" << repeat
                              << ", time=" << elapsed << " sec\n";

                    out << n << ',' << threads << ',' << repeat << ',' << elapsed << ','
                        << checksum << ',' << 0.0 << '\n';
                }
            }
        }

        out.flush();

        fs::path summary_csv = config.output_csv.parent_path() / "matrix_vector_summary.csv";
        std::ofstream summary(summary_csv);
        if (!summary) {
            throw std::runtime_error("Cannot open summary file: " + summary_csv.string());
        }
        summary << "matrix_size,threads,best_time_sec,speedup\n";
        summary << std::fixed << std::setprecision(6);

        for (std::int64_t n : config.sizes) {
            double t1 = 0.0;
            for (int threads : config.threads) {
                double best_time = std::numeric_limits<double>::max();
                std::ifstream raw(config.output_csv);
                std::string line;
                std::getline(raw, line);
                while (std::getline(raw, line)) {
                    std::replace(line.begin(), line.end(), ',', ' ');
                    std::istringstream iss(line);
                    std::int64_t size = 0;
                    int th = 0;
                    int rep = 0;
                    double time_sec = 0.0, checksum = 0.0, speedup = 0.0;
                    if (iss >> size >> th >> rep >> time_sec >> checksum >> speedup) {
                        if (size == n && th == threads) {
                            best_time = std::min(best_time, time_sec);
                        }
                    }
                }
                if (threads == 1) {
                    t1 = best_time;
                }
                const double sp = (best_time > 0.0) ? (t1 / best_time) : 0.0;
                summary << n << ',' << threads << ',' << best_time << ',' << sp << '\n';
            }
        }

        std::cout << "Saved raw results to: " << config.output_csv << "\n";
        std::cout << "Saved summary to: " << (config.output_csv.parent_path() / "matrix_vector_summary.csv") << "\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "ERROR: " << ex.what() << '\n';
        return 1;
    }
}
