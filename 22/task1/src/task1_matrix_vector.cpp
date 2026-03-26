#include <omp.h>

#include <chrono>
#include <cmath>
#include <cstddef>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;

struct Options {
    std::size_t n = 2000;
    int threads = 1;
    int repeats = 1;
    std::string csv_path;
};

struct Metrics {
    double init_seconds = 0.0;
    double multiply_seconds = 0.0;
    double total_seconds = 0.0;
    double checksum = 0.0;
    double memory_gib = 0.0;
};

[[noreturn]] void fail(const std::string& message) {
    throw std::runtime_error(message);
}

Options parse_args(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto require_value = [&](const std::string& name) -> std::string {
            if (i + 1 >= argc) {
                fail("Missing value for " + name);
            }
            return argv[++i];
        };

        if (arg == "--n") {
            options.n = static_cast<std::size_t>(std::stoull(require_value(arg)));
        } else if (arg == "--threads") {
            options.threads = std::stoi(require_value(arg));
        } else if (arg == "--repeats") {
            options.repeats = std::stoi(require_value(arg));
        } else if (arg == "--csv") {
            options.csv_path = require_value(arg);
        } else if (arg == "--help" || arg == "-h") {
            std::cout
                << "Usage: task1_matvec [--n N] [--threads T] [--repeats R] [--csv FILE]\n"
                << "Dense matrix-vector multiplication with OpenMP and parallel initialization.\n";
            std::exit(0);
        } else {
            fail("Unknown argument: " + arg);
        }
    }

    if (options.n == 0) {
        fail("N must be positive");
    }
    if (options.threads <= 0) {
        fail("threads must be positive");
    }
    if (options.repeats <= 0) {
        fail("repeats must be positive");
    }
    return options;
}

std::size_t checked_matrix_elements(std::size_t n) {
    if (n != 0 && n > std::numeric_limits<std::size_t>::max() / n) {
        fail("Matrix size overflows size_t");
    }
    return n * n;
}

double to_gib(std::size_t bytes) {
    return static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0);
}

void write_csv_header_if_needed(const std::string& path) {
    std::ifstream in(path);
    if (in.good() && in.peek() != std::ifstream::traits_type::eof()) {
        return;
    }
    std::ofstream out(path, std::ios::app);
    out << "n,threads,repeats,init_seconds,multiply_seconds,total_seconds,checksum,memory_gib\n";
}

void append_csv(const std::string& path, const Options& options, const Metrics& metrics) {
    if (path.empty()) {
        return;
    }
    write_csv_header_if_needed(path);
    std::ofstream out(path, std::ios::app);
    out << options.n << ','
        << options.threads << ','
        << options.repeats << ','
        << std::setprecision(12) << metrics.init_seconds << ','
        << metrics.multiply_seconds << ','
        << metrics.total_seconds << ','
        << metrics.checksum << ','
        << metrics.memory_gib << '\n';
}

Metrics run_benchmark(const Options& options) {
    const std::size_t matrix_elements = checked_matrix_elements(options.n);
    const std::size_t total_bytes = matrix_elements * sizeof(double) + 2 * options.n * sizeof(double);

    omp_set_num_threads(options.threads);

    std::vector<double> matrix;
    std::vector<double> x;
    std::vector<double> y;

    try {
        matrix.resize(matrix_elements);
        x.resize(options.n);
        y.resize(options.n);
    } catch (const std::bad_alloc&) {
        fail("Memory allocation failed. Try a smaller N or run on a server with more RAM.");
    }

    double sum_init = 0.0;
    double sum_multiply = 0.0;
    double sum_total = 0.0;
    double checksum = 0.0;

    for (int repeat = 0; repeat < options.repeats; ++repeat) {
        const auto total_begin = Clock::now();

        const auto init_begin = Clock::now();
#pragma omp parallel for schedule(static)
        for (std::ptrdiff_t i = 0; i < static_cast<std::ptrdiff_t>(options.n); ++i) {
            x[static_cast<std::size_t>(i)] = 1.0 + static_cast<double>(i % 1024) * 1.0e-4;
            y[static_cast<std::size_t>(i)] = 0.0;
        }

#pragma omp parallel for schedule(static)
        for (std::ptrdiff_t index = 0; index < static_cast<std::ptrdiff_t>(matrix_elements); ++index) {
            const auto i = static_cast<std::size_t>(index) / options.n;
            const auto j = static_cast<std::size_t>(index) % options.n;
            matrix[static_cast<std::size_t>(index)] = 1.0 / (1.0 + static_cast<double>((i + j) % 2048));
        }
        const auto init_end = Clock::now();

        const auto mul_begin = Clock::now();
#pragma omp parallel for schedule(static)
        for (std::ptrdiff_t i = 0; i < static_cast<std::ptrdiff_t>(options.n); ++i) {
            const std::size_t row = static_cast<std::size_t>(i) * options.n;
            double acc = 0.0;
            for (std::size_t j = 0; j < options.n; ++j) {
                acc += matrix[row + j] * x[j];
            }
            y[static_cast<std::size_t>(i)] = acc;
        }
        const auto mul_end = Clock::now();

        double run_checksum = 0.0;
#pragma omp parallel for reduction(+:run_checksum) schedule(static)
        for (std::ptrdiff_t i = 0; i < static_cast<std::ptrdiff_t>(options.n); ++i) {
            run_checksum += y[static_cast<std::size_t>(i)];
        }

        const auto total_end = Clock::now();

        const double init_seconds = std::chrono::duration<double>(init_end - init_begin).count();
        const double multiply_seconds = std::chrono::duration<double>(mul_end - mul_begin).count();
        const double total_seconds = std::chrono::duration<double>(total_end - total_begin).count();

        sum_init += init_seconds;
        sum_multiply += multiply_seconds;
        sum_total += total_seconds;
        checksum = run_checksum;
    }

    Metrics metrics;
    metrics.init_seconds = sum_init / static_cast<double>(options.repeats);
    metrics.multiply_seconds = sum_multiply / static_cast<double>(options.repeats);
    metrics.total_seconds = sum_total / static_cast<double>(options.repeats);
    metrics.checksum = checksum;
    metrics.memory_gib = to_gib(total_bytes);
    return metrics;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        const Options options = parse_args(argc, argv);
        const Metrics metrics = run_benchmark(options);
        append_csv(options.csv_path, options, metrics);

        std::cout << std::fixed << std::setprecision(6);
        std::cout << "task1_matvec\n";
        std::cout << "n=" << options.n << '\n';
        std::cout << "threads=" << options.threads << '\n';
        std::cout << "repeats=" << options.repeats << '\n';
        std::cout << "memory_gib=" << metrics.memory_gib << '\n';
        std::cout << "init_seconds=" << metrics.init_seconds << '\n';
        std::cout << "multiply_seconds=" << metrics.multiply_seconds << '\n';
        std::cout << "total_seconds=" << metrics.total_seconds << '\n';
        std::cout << "checksum=" << metrics.checksum << '\n';
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }
}
