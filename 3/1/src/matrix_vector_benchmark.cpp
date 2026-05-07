#include <algorithm>
#include <chrono>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

using Clock = std::chrono::steady_clock;

struct BenchmarkConfig {
    std::vector<std::size_t> sizes{20000, 40000};
    std::vector<unsigned> thread_counts{1, 2, 4, 7, 8, 16, 20, 40};
    fs::path output_csv{"results/task1_matrix_vector.csv"};
};

struct BenchmarkRow {
    std::size_t matrix_size{};
    unsigned threads{};
    double init_seconds{};
    double multiply_seconds{};
    double total_seconds{};
    double multiply_speedup{};
    double total_speedup{};
    double checksum{};
};

std::vector<std::string> split(const std::string& text, char delimiter) {
    std::vector<std::string> parts;
    std::stringstream ss(text);
    std::string item;
    while (std::getline(ss, item, delimiter)) {
        if (!item.empty()) {
            parts.push_back(item);
        }
    }
    return parts;
}

std::vector<std::size_t> parse_sizes(const std::string& text) {
    std::vector<std::size_t> values;
    for (const auto& part : split(text, ',')) {
        values.push_back(static_cast<std::size_t>(std::stoull(part)));
    }
    if (values.empty()) {
        throw std::invalid_argument("empty size list");
    }
    return values;
}

std::vector<unsigned> parse_threads(const std::string& text) {
    std::vector<unsigned> values;
    for (const auto& part : split(text, ',')) {
        const unsigned value = static_cast<unsigned>(std::stoul(part));
        if (value == 0) {
            throw std::invalid_argument("thread count must be positive");
        }
        values.push_back(value);
    }
    if (values.empty()) {
        throw std::invalid_argument("empty thread list");
    }
    return values;
}

BenchmarkConfig parse_arguments(int argc, char** argv) {
    BenchmarkConfig config;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--sizes" && i + 1 < argc) {
            config.sizes = parse_sizes(argv[++i]);
        } else if (arg == "--threads" && i + 1 < argc) {
            config.thread_counts = parse_threads(argv[++i]);
        } else if (arg == "--output" && i + 1 < argc) {
            config.output_csv = argv[++i];
        } else if (arg == "--help") {
            std::cout << "Usage:\n"
                      << "  ./matrix_vector_benchmark [--sizes 20000,40000] "
                      << "[--threads 1,2,4,7,8,16,20,40] "
                      << "[--output results/task1_matrix_vector.csv]\n";
            std::exit(0);
        } else {
            throw std::invalid_argument("unknown or incomplete argument: " + arg);
        }
    }
    return config;
}

std::pair<std::size_t, std::size_t> block_range(std::size_t total, unsigned thread_index, unsigned thread_count) {
    const std::size_t begin = total * thread_index / thread_count;
    const std::size_t end = total * (thread_index + 1) / thread_count;
    return {begin, end};
}

void parallel_initialize(std::vector<double>& matrix,
                         std::vector<double>& vector,
                         unsigned thread_count,
                         std::size_t n) {
    std::vector<std::thread> workers;
    workers.reserve(thread_count);

    for (unsigned tid = 0; tid < thread_count; ++tid) {
        workers.emplace_back([&, tid] {
            const auto [row_begin, row_end] = block_range(n, tid, thread_count);

            for (std::size_t i = row_begin; i < row_end; ++i) {
                vector[i] = 1.0 + static_cast<double>(i % 97) * 0.001;

                const std::size_t row_offset = i * n;
                for (std::size_t j = 0; j < n; ++j) {
                    matrix[row_offset + j] = 0.5
                        + static_cast<double>((i + j) % 131) * 0.0001;
                }
            }
        });
    }

    for (auto& worker : workers) {
        worker.join();
    }
}

void parallel_multiply(const std::vector<double>& matrix,
                       const std::vector<double>& vector,
                       std::vector<double>& result,
                       unsigned thread_count,
                       std::size_t n) {
    std::vector<std::thread> workers;
    workers.reserve(thread_count);

    for (unsigned tid = 0; tid < thread_count; ++tid) {
        workers.emplace_back([&, tid] {
            const auto [row_begin, row_end] = block_range(n, tid, thread_count);

            for (std::size_t i = row_begin; i < row_end; ++i) {
                const std::size_t row_offset = i * n;
                double sum = 0.0;
                for (std::size_t j = 0; j < n; ++j) {
                    sum += matrix[row_offset + j] * vector[j];
                }
                result[i] = sum;
            }
        });
    }

    for (auto& worker : workers) {
        worker.join();
    }
}

double checksum(const std::vector<double>& result) {
    return std::accumulate(result.begin(), result.end(), 0.0);
}

double seconds_since(const Clock::time_point& start, const Clock::time_point& finish) {
    return std::chrono::duration<double>(finish - start).count();
}

std::vector<BenchmarkRow> benchmark_size(std::size_t n, const std::vector<unsigned>& thread_counts) {
    const std::size_t matrix_elements = n * n;
    std::vector<double> matrix(matrix_elements);
    std::vector<double> vector(n);
    std::vector<double> result(n);
    std::vector<BenchmarkRow> rows;

    double baseline_multiply = 0.0;
    double baseline_total = 0.0;

    for (unsigned thread_count : thread_counts) {
        std::fill(result.begin(), result.end(), 0.0);

        const auto init_start = Clock::now();
        parallel_initialize(matrix, vector, thread_count, n);
        const auto init_finish = Clock::now();

        const auto multiply_start = Clock::now();
        parallel_multiply(matrix, vector, result, thread_count, n);
        const auto multiply_finish = Clock::now();

        const double init_seconds = seconds_since(init_start, init_finish);
        const double multiply_seconds = seconds_since(multiply_start, multiply_finish);
        const double total_seconds = init_seconds + multiply_seconds;
        const double result_checksum = checksum(result);

        if (thread_count == 1) {
            baseline_multiply = multiply_seconds;
            baseline_total = total_seconds;
        }

        rows.push_back(BenchmarkRow{
            n,
            thread_count,
            init_seconds,
            multiply_seconds,
            total_seconds,
            baseline_multiply > 0.0 ? baseline_multiply / multiply_seconds : 0.0,
            baseline_total > 0.0 ? baseline_total / total_seconds : 0.0,
            result_checksum
        });

        std::cout << "N=" << n
                  << ", threads=" << thread_count
                  << ", init=" << init_seconds << " s"
                  << ", multiply=" << multiply_seconds << " s"
                  << ", speedup=" << rows.back().multiply_speedup
                  << '\n';
    }

    return rows;
}

void write_csv_header(std::ofstream& out) {
    out << "matrix_size,threads,init_seconds,multiply_seconds,total_seconds,"
           "multiply_speedup,total_speedup,checksum\n";
}

void write_csv_row(std::ofstream& out, const BenchmarkRow& row) {
    out << row.matrix_size << ','
        << row.threads << ','
        << std::setprecision(17) << row.init_seconds << ','
        << std::setprecision(17) << row.multiply_seconds << ','
        << std::setprecision(17) << row.total_seconds << ','
        << std::setprecision(17) << row.multiply_speedup << ','
        << std::setprecision(17) << row.total_speedup << ','
        << std::setprecision(17) << row.checksum << '\n';
}

void print_memory_warning(std::size_t n) {
    const long double matrix_gib = static_cast<long double>(n) * static_cast<long double>(n)
        * sizeof(double) / 1024.0L / 1024.0L / 1024.0L;
    const long double total_gib = matrix_gib
        + static_cast<long double>(2 * n) * sizeof(double) / 1024.0L / 1024.0L / 1024.0L;

    std::cout << "Matrix " << n << "x" << n
              << " requires about " << std::fixed << std::setprecision(2)
              << matrix_gib << " GiB for the matrix, total about "
              << total_gib << " GiB with vectors.\n";
}

int main(int argc, char** argv) {
    try {
        const BenchmarkConfig config = parse_arguments(argc, argv);

        fs::create_directories(config.output_csv.parent_path());
        std::ofstream out(config.output_csv);
        if (!out) {
            throw std::runtime_error("cannot open output file: " + config.output_csv.string());
        }
        write_csv_header(out);

        for (std::size_t n : config.sizes) {
            print_memory_warning(n);
            const auto rows = benchmark_size(n, config.thread_counts);
            for (const BenchmarkRow& row : rows) {
                write_csv_row(out, row);
            }
            out.flush();
        }

        std::cout << "Results saved to " << config.output_csv << '\n';
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
