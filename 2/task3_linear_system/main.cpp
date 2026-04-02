#include <omp.h>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

enum class Variant {
    ManyParallelFor = 1,
    SingleParallelRegion = 2
};

struct Config {
    std::size_t n = 10000000;
    double eps = 1e-5;
    int max_iters = 100000;
    int repeats = 3;
    std::vector<int> threads;
    fs::path variants_csv = "tables/variants_summary.csv";
    fs::path schedule_csv = "tables/schedule_summary.csv";
};

static void ensure_parent_dir(const fs::path& path) {
    if (path.has_parent_path()) {
        fs::create_directories(path.parent_path());
    }
}

static double residual_value(double sum_x, std::size_t n) {
    return std::abs(sum_x - static_cast<double>(n));
}

static double solve_variant(std::size_t n, double eps, int max_iters, int threads, Variant variant, omp_sched_t sched_kind, int chunk_size, int& iters_done, double& final_residual) {
    omp_set_num_threads(threads);
    omp_set_schedule(sched_kind, chunk_size);

    std::vector<double> x(n, 0.0);
    std::vector<double> x_next(n, 0.0);

    const double tau = 0.9 / (static_cast<double>(n) + 1.0);
    const double start = omp_get_wtime();
    iters_done = 0;
    final_residual = std::numeric_limits<double>::infinity();

    if (variant == Variant::ManyParallelFor) {
        for (int iter = 0; iter < max_iters; ++iter) {
            double sum_x = 0.0;
            #pragma omp parallel for reduction(+:sum_x) schedule(runtime)
            for (std::size_t i = 0; i < n; ++i) {
                sum_x += x[i];
            }

            final_residual = residual_value(sum_x, n);
            if (final_residual < eps) {
                iters_done = iter;
                break;
            }

            #pragma omp parallel for schedule(runtime)
            for (std::size_t i = 0; i < n; ++i) {
                const double ax_i = sum_x + x[i];
                x_next[i] = x[i] - tau * (ax_i - (static_cast<double>(n) + 1.0));
            }

            #pragma omp parallel for schedule(runtime)
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

                #pragma omp for reduction(+:sum_x) schedule(runtime)
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

                #pragma omp for schedule(runtime)
                for (std::size_t i = 0; i < n; ++i) {
                    const double ax_i = sum_x + x[i];
                    x_next[i] = x[i] - tau * (ax_i - (static_cast<double>(n) + 1.0));
                }

                #pragma omp for schedule(runtime)
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

static std::string sched_name(omp_sched_t s) {
    switch (s) {
        case omp_sched_static: return "static";
        case omp_sched_dynamic: return "dynamic";
        case omp_sched_guided: return "guided";
        case omp_sched_auto: return "auto";
        default: return "unknown";
    }
}

int main(int argc, char** argv) {
    Config cfg;
    const int max_threads = omp_get_max_threads();
    for (int t = 1; t <= max_threads; ++t) {
        cfg.threads.push_back(t);
    }

    try {
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--n" && i + 1 < argc) {
                cfg.n = static_cast<std::size_t>(std::stoull(argv[++i]));
            } else if (arg == "--eps" && i + 1 < argc) {
                cfg.eps = std::stod(argv[++i]);
            } else if (arg == "--max-iters" && i + 1 < argc) {
                cfg.max_iters = std::stoi(argv[++i]);
            } else if (arg == "--repeats" && i + 1 < argc) {
                cfg.repeats = std::stoi(argv[++i]);
            } else if (arg == "--variants-out" && i + 1 < argc) {
                cfg.variants_csv = argv[++i];
            } else if (arg == "--schedule-out" && i + 1 < argc) {
                cfg.schedule_csv = argv[++i];
            } else {
                throw std::runtime_error("Unknown argument: " + arg);
            }
        }

        ensure_parent_dir(cfg.variants_csv);
        ensure_parent_dir(cfg.schedule_csv);

        std::ofstream variants(cfg.variants_csv);
        std::ofstream sched(cfg.schedule_csv);
        if (!variants || !sched) {
            throw std::runtime_error("Cannot open output CSV file(s)");
        }

        variants << "variant,threads,time_sec,speedup,efficiency,iterations,residual\n";
        variants << std::fixed << std::setprecision(9);

        const std::vector<Variant> variants_list{Variant::ManyParallelFor, Variant::SingleParallelRegion};

        for (Variant v : variants_list) {
            double t1 = 0.0;
            for (int threads : cfg.threads) {
                double best_time = std::numeric_limits<double>::max();
                int best_iters = 0;
                double best_res = 0.0;
                for (int rep = 0; rep < cfg.repeats; ++rep) {
                    int iters = 0;
                    double residual = 0.0;
                    double elapsed = solve_variant(cfg.n, cfg.eps, cfg.max_iters, threads, v,
                                                   omp_sched_static, 0, iters, residual);
                    if (elapsed < best_time) {
                        best_time = elapsed;
                        best_iters = iters;
                        best_res = residual;
                    }
                }

                if (threads == 1) {
                    t1 = best_time;
                }

                const double sp = t1 / best_time;
                const double ef = sp / static_cast<double>(threads);
                variants << (v == Variant::ManyParallelFor ? "parallel_for_each_loop" : "single_parallel_region")
                         << ',' << threads << ',' << best_time << ',' << sp << ',' << ef
                         << ',' << best_iters << ',' << best_res << '\n';
            }
        }

        const int study_threads = std::max(1, std::min(8, max_threads));
        const Variant study_variant = Variant::SingleParallelRegion;
        const std::vector<std::pair<omp_sched_t, int>> schedules = {
            {omp_sched_static, 0},
            {omp_sched_static, 1},
            {omp_sched_dynamic, 1},
            {omp_sched_dynamic, 64},
            {omp_sched_guided, 1},
            {omp_sched_guided, 64}
        };

        sched << "threads,variant,schedule,chunk,time_sec,iterations,residual\n";
        sched << std::fixed << std::setprecision(9);

        for (const auto& item : schedules) {
            omp_sched_t kind = item.first;
            int chunk = item.second;
            double best_time = std::numeric_limits<double>::max();
            int best_iters = 0;
            double best_res = 0.0;
            for (int rep = 0; rep < cfg.repeats; ++rep) {
                int iters = 0;
                double residual = 0.0;
                double elapsed = solve_variant(cfg.n, cfg.eps, cfg.max_iters, study_threads, study_variant,
                                               kind, chunk, iters, residual);
                if (elapsed < best_time) {
                    best_time = elapsed;
                    best_iters = iters;
                    best_res = residual;
                }
            }

            sched << study_threads << ','
                  << "single_parallel_region" << ','
                  << sched_name(kind) << ','
                  << chunk << ','
                  << best_time << ','
                  << best_iters << ','
                  << best_res << '\n';
        }

        std::cout << "Saved CSV files:\n"
                  << "  " << cfg.variants_csv << '\n'
                  << "  " << cfg.schedule_csv << '\n';
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "ERROR: " << ex.what() << '\n';
        return 1;
    }
}
