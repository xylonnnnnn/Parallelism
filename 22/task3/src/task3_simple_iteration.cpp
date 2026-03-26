#include <omp.h>

#include <chrono>
#include <cmath>
#include <cstddef>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;

struct Options {
    std::size_t n = 1000000;
    int threads = 1;
    int repeats = 1;
    int variant = 1;
    double eps = 1.0e-5;
    long long max_iters = 1000000;
    double tau = -1.0;  // auto
    std::string schedule = "static";
    int chunk = 0;
    std::string csv_path;
};

struct Result {
    long long iterations = 0;
    double seconds = 0.0;
    double residual = 0.0;
    double linf_error = 0.0;
    bool converged = false;
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
        } else if (arg == "--variant") {
            options.variant = std::stoi(require_value(arg));
        } else if (arg == "--eps") {
            options.eps = std::stod(require_value(arg));
        } else if (arg == "--max-iters") {
            options.max_iters = std::stoll(require_value(arg));
        } else if (arg == "--tau") {
            options.tau = std::stod(require_value(arg));
        } else if (arg == "--schedule") {
            options.schedule = require_value(arg);
        } else if (arg == "--chunk") {
            options.chunk = std::stoi(require_value(arg));
        } else if (arg == "--csv") {
            options.csv_path = require_value(arg);
        } else if (arg == "--help" || arg == "-h") {
            std::cout
                << "Usage: task3_solver [--n N] [--threads T] [--variant 1|2] [--eps E] [--max-iters M] "
                << "[--tau VALUE] [--schedule static|dynamic|guided|auto] [--chunk C] [--repeats R] [--csv FILE]\n";
            std::exit(0);
        } else {
            fail("Unknown argument: " + arg);
        }
    }

    if (options.n == 0) {
        fail("n must be positive");
    }
    if (options.threads <= 0) {
        fail("threads must be positive");
    }
    if (options.repeats <= 0) {
        fail("repeats must be positive");
    }
    if (options.variant != 1 && options.variant != 2) {
        fail("variant must be 1 or 2");
    }
    if (options.eps <= 0.0) {
        fail("eps must be positive");
    }
    if (options.max_iters <= 0) {
        fail("max-iters must be positive");
    }
    if (options.chunk < 0) {
        fail("chunk must be >= 0");
    }
    return options;
}

omp_sched_t parse_schedule(const std::string& schedule_name) {
    if (schedule_name == "static") {
        return omp_sched_static;
    }
    if (schedule_name == "dynamic") {
        return omp_sched_dynamic;
    }
    if (schedule_name == "guided") {
        return omp_sched_guided;
    }
    if (schedule_name == "auto") {
        return omp_sched_auto;
    }
    fail("Unknown schedule: " + schedule_name);
}

double choose_tau(std::size_t n) {
    // For A = I + J, eigenvalues are 1 and n + 1.
    // The minimax choice for simple iteration is 2 / (lambda_min + lambda_max).
    return 2.0 / (static_cast<double>(n) + 2.0);
}

void write_csv_header_if_needed(const std::string& path) {
    std::ifstream in(path);
    if (in.good() && in.peek() != std::ifstream::traits_type::eof()) {
        return;
    }
    std::ofstream out(path, std::ios::app);
    out << "n,threads,variant,repeats,eps,max_iters,tau,schedule,chunk,iterations,seconds,residual,linf_error,converged\n";
}

void append_csv(const std::string& path, const Options& options, const Result& result, double tau) {
    if (path.empty()) {
        return;
    }
    write_csv_header_if_needed(path);
    std::ofstream out(path, std::ios::app);
    out << options.n << ','
        << options.threads << ','
        << options.variant << ','
        << options.repeats << ','
        << options.eps << ','
        << options.max_iters << ','
        << std::setprecision(12) << tau << ','
        << options.schedule << ','
        << options.chunk << ','
        << result.iterations << ','
        << result.seconds << ','
        << result.residual << ','
        << result.linf_error << ','
        << (result.converged ? 1 : 0) << '\n';
}

Result solve_variant1(const Options& options, double tau) {
    omp_set_num_threads(options.threads);
    omp_set_schedule(parse_schedule(options.schedule), options.chunk);

    const double b = static_cast<double>(options.n) + 1.0;
    const double norm_b = std::sqrt(static_cast<double>(options.n)) * b;

    std::vector<double> x(options.n, 0.0);
    std::vector<double> next_x(options.n, 0.0);

    Result result;
    const auto begin = Clock::now();

    for (long long iter = 0; iter < options.max_iters; ++iter) {
        double sum_x = 0.0;
#pragma omp parallel for reduction(+:sum_x) schedule(runtime)
        for (std::ptrdiff_t i = 0; i < static_cast<std::ptrdiff_t>(options.n); ++i) {
            sum_x += x[static_cast<std::size_t>(i)];
        }

        double residual_sq = 0.0;
#pragma omp parallel for reduction(+:residual_sq) schedule(runtime)
        for (std::ptrdiff_t i = 0; i < static_cast<std::ptrdiff_t>(options.n); ++i) {
            const std::size_t idx = static_cast<std::size_t>(i);
            const double residual_i = sum_x + x[idx] - b;
            residual_sq += residual_i * residual_i;
            next_x[idx] = x[idx] - tau * residual_i;
        }

        result.iterations = iter + 1;
        result.residual = std::sqrt(residual_sq) / norm_b;
        x.swap(next_x);
        if (result.residual < options.eps) {
            result.converged = true;
            break;
        }
    }

    double linf_error = 0.0;
#pragma omp parallel for reduction(max:linf_error) schedule(runtime)
    for (std::ptrdiff_t i = 0; i < static_cast<std::ptrdiff_t>(options.n); ++i) {
        const double diff = std::fabs(x[static_cast<std::size_t>(i)] - 1.0);
        if (diff > linf_error) {
            linf_error = diff;
        }
    }
    result.linf_error = linf_error;

    result.seconds = std::chrono::duration<double>(Clock::now() - begin).count();
    return result;
}

Result solve_variant2(const Options& options, double tau) {
    omp_set_num_threads(options.threads);
    omp_set_schedule(parse_schedule(options.schedule), options.chunk);

    const double b = static_cast<double>(options.n) + 1.0;
    const double norm_b = std::sqrt(static_cast<double>(options.n)) * b;

    std::vector<double> x(options.n, 0.0);
    std::vector<double> next_x(options.n, 0.0);

    Result result;
    bool done = false;
    double sum_x = 0.0;
    double residual_sq = 0.0;

    const auto begin = Clock::now();

#pragma omp parallel shared(x, next_x, result, done, sum_x, residual_sq)
    {
        while (true) {
#pragma omp single
            {
                sum_x = 0.0;
                residual_sq = 0.0;
            }
#pragma omp barrier

#pragma omp for reduction(+:sum_x) schedule(runtime)
            for (std::ptrdiff_t i = 0; i < static_cast<std::ptrdiff_t>(options.n); ++i) {
                sum_x += x[static_cast<std::size_t>(i)];
            }

#pragma omp for reduction(+:residual_sq) schedule(runtime)
            for (std::ptrdiff_t i = 0; i < static_cast<std::ptrdiff_t>(options.n); ++i) {
                const std::size_t idx = static_cast<std::size_t>(i);
                const double residual_i = sum_x + x[idx] - b;
                residual_sq += residual_i * residual_i;
                next_x[idx] = x[idx] - tau * residual_i;
            }

#pragma omp single
            {
                ++result.iterations;
                result.residual = std::sqrt(residual_sq) / norm_b;
                x.swap(next_x);
                done = (result.residual < options.eps) || (result.iterations >= options.max_iters);
                if (result.residual < options.eps) {
                    result.converged = true;
                }
            }
#pragma omp barrier
            if (done) {
                break;
            }
        }
    }

    double linf_error = 0.0;
#pragma omp parallel for reduction(max:linf_error) schedule(runtime)
    for (std::ptrdiff_t i = 0; i < static_cast<std::ptrdiff_t>(options.n); ++i) {
        const double diff = std::fabs(x[static_cast<std::size_t>(i)] - 1.0);
        if (diff > linf_error) {
            linf_error = diff;
        }
    }
    result.linf_error = linf_error;

    result.seconds = std::chrono::duration<double>(Clock::now() - begin).count();
    return result;
}

Result benchmark(const Options& options, double tau) {
    double total_seconds = 0.0;
    double total_residual = 0.0;
    double total_error = 0.0;
    long long total_iterations = 0;
    bool converged = true;

    Result last;
    for (int repeat = 0; repeat < options.repeats; ++repeat) {
        Result current = (options.variant == 1) ? solve_variant1(options, tau) : solve_variant2(options, tau);
        total_seconds += current.seconds;
        total_residual += current.residual;
        total_error += current.linf_error;
        total_iterations += current.iterations;
        converged = converged && current.converged;
        last = current;
    }

    last.seconds = total_seconds / static_cast<double>(options.repeats);
    last.residual = total_residual / static_cast<double>(options.repeats);
    last.linf_error = total_error / static_cast<double>(options.repeats);
    last.iterations = total_iterations / options.repeats;
    last.converged = converged;
    return last;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        const Options options = parse_args(argc, argv);
        const double tau = (options.tau > 0.0) ? options.tau : choose_tau(options.n);
        const Result result = benchmark(options, tau);
        append_csv(options.csv_path, options, result, tau);

        std::cout << std::fixed << std::setprecision(10);
        std::cout << "task3_solver\n";
        std::cout << "n=" << options.n << '\n';
        std::cout << "threads=" << options.threads << '\n';
        std::cout << "variant=" << options.variant << '\n';
        std::cout << "repeats=" << options.repeats << '\n';
        std::cout << "eps=" << options.eps << '\n';
        std::cout << "tau=" << tau << '\n';
        std::cout << "schedule=" << options.schedule << '\n';
        std::cout << "chunk=" << options.chunk << '\n';
        std::cout << "iterations=" << result.iterations << '\n';
        std::cout << "seconds=" << result.seconds << '\n';
        std::cout << "residual=" << result.residual << '\n';
        std::cout << "linf_error=" << result.linf_error << '\n';
        std::cout << "converged=" << (result.converged ? 1 : 0) << '\n';
        return result.converged ? 0 : 2;
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }
}
