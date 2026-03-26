#include <omp.h>

#include <chrono>
#include <cmath>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

using Clock = std::chrono::steady_clock;

struct Options {
    long long nsteps = 40000000LL;
    int threads = 1;
    int repeats = 1;
    std::string csv_path;
};

struct Metrics {
    double serial_seconds = 0.0;
    double omp_seconds = 0.0;
    double serial_value = 0.0;
    double omp_value = 0.0;
    double abs_error = 0.0;
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

        if (arg == "--nsteps") {
            options.nsteps = std::stoll(require_value(arg));
        } else if (arg == "--threads") {
            options.threads = std::stoi(require_value(arg));
        } else if (arg == "--repeats") {
            options.repeats = std::stoi(require_value(arg));
        } else if (arg == "--csv") {
            options.csv_path = require_value(arg);
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: task2_integration [--nsteps N] [--threads T] [--repeats R] [--csv FILE]\n";
            std::exit(0);
        } else {
            fail("Unknown argument: " + arg);
        }
    }

    if (options.nsteps <= 0) {
        fail("nsteps must be positive");
    }
    if (options.threads <= 0) {
        fail("threads must be positive");
    }
    if (options.repeats <= 0) {
        fail("repeats must be positive");
    }
    return options;
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

double integrate_omp(long long nsteps, int threads) {
    const double step = 1.0 / static_cast<double>(nsteps);
    double global_sum = 0.0;

#pragma omp parallel num_threads(threads)
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

void write_csv_header_if_needed(const std::string& path) {
    std::ifstream in(path);
    if (in.good() && in.peek() != std::ifstream::traits_type::eof()) {
        return;
    }
    std::ofstream out(path, std::ios::app);
    out << "nsteps,threads,repeats,serial_seconds,omp_seconds,speedup,serial_value,omp_value,abs_error\n";
}

void append_csv(const std::string& path, const Options& options, const Metrics& metrics) {
    if (path.empty()) {
        return;
    }
    write_csv_header_if_needed(path);
    std::ofstream out(path, std::ios::app);
    const double speedup = metrics.serial_seconds / metrics.omp_seconds;
    out << options.nsteps << ','
        << options.threads << ','
        << options.repeats << ','
        << std::setprecision(12) << metrics.serial_seconds << ','
        << metrics.omp_seconds << ','
        << speedup << ','
        << metrics.serial_value << ','
        << metrics.omp_value << ','
        << metrics.abs_error << '\n';
}

Metrics benchmark(const Options& options) {
    double serial_seconds = 0.0;
    double omp_seconds = 0.0;
    double serial_value = 0.0;
    double omp_value = 0.0;

    for (int repeat = 0; repeat < options.repeats; ++repeat) {
        const auto serial_begin = Clock::now();
        serial_value = integrate_serial(options.nsteps);
        const auto serial_end = Clock::now();

        const auto omp_begin = Clock::now();
        omp_value = integrate_omp(options.nsteps, options.threads);
        const auto omp_end = Clock::now();

        serial_seconds += std::chrono::duration<double>(serial_end - serial_begin).count();
        omp_seconds += std::chrono::duration<double>(omp_end - omp_begin).count();
    }

    Metrics metrics;
    metrics.serial_seconds = serial_seconds / static_cast<double>(options.repeats);
    metrics.omp_seconds = omp_seconds / static_cast<double>(options.repeats);
    metrics.serial_value = serial_value;
    metrics.omp_value = omp_value;
    metrics.abs_error = std::abs(serial_value - omp_value);
    return metrics;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        const Options options = parse_args(argc, argv);
        const Metrics metrics = benchmark(options);
        append_csv(options.csv_path, options, metrics);

        std::cout << std::fixed << std::setprecision(12);
        std::cout << "task2_integration\n";
        std::cout << "nsteps=" << options.nsteps << '\n';
        std::cout << "threads=" << options.threads << '\n';
        std::cout << "repeats=" << options.repeats << '\n';
        std::cout << "serial_seconds=" << metrics.serial_seconds << '\n';
        std::cout << "omp_seconds=" << metrics.omp_seconds << '\n';
        std::cout << "speedup=" << metrics.serial_seconds / metrics.omp_seconds << '\n';
        std::cout << "serial_value=" << metrics.serial_value << '\n';
        std::cout << "omp_value=" << metrics.omp_value << '\n';
        std::cout << "abs_error=" << metrics.abs_error << '\n';
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }
}
