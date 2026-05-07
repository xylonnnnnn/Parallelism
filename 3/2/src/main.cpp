#include "task_server.hpp"

#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

struct ResultRow {
    size_t task_id{};
    std::string operation;
    double x{};
    double y{};
    double result{};
};

using Server = lab::TaskServer<ResultRow>;

std::string csv_name(const std::string& operation) {
    return "results/client_" + operation + ".csv";
}

void write_header(std::ofstream& out) {
    out << "task_id,operation,x,y,result\n";
}

void write_row(std::ofstream& out, const ResultRow& row) {
    out << row.task_id << ','
        << row.operation << ','
        << std::setprecision(17) << row.x << ','
        << std::setprecision(17) << row.y << ','
        << std::setprecision(17) << row.result << '\n';
}

void client_sin(Server& server, int n) {
    std::mt19937_64 gen(std::random_device{}());
    std::uniform_real_distribution<double> dist(-1000.0, 1000.0);

    std::ofstream out(csv_name("sin"));
    write_header(out);

    for (int i = 0; i < n; ++i) {
        const double x = dist(gen);
        const size_t id = server.add_task([x] {
            return ResultRow{0, "sin", x, 0.0, std::sin(x)};
        });

        ResultRow row = server.request_result(id);
        row.task_id = id;
        write_row(out, row);
    }
}

void client_sqrt(Server& server, int n) {
    std::mt19937_64 gen(std::random_device{}());
    std::uniform_real_distribution<double> dist(0.0, 1'000'000.0);

    std::ofstream out(csv_name("sqrt"));
    write_header(out);

    for (int i = 0; i < n; ++i) {
        const double x = dist(gen);
        const size_t id = server.add_task([x] {
            return ResultRow{0, "sqrt", x, 0.0, std::sqrt(x)};
        });

        ResultRow row = server.request_result(id);
        row.task_id = id;
        write_row(out, row);
    }
}

void client_pow(Server& server, int n) {
    std::mt19937_64 gen(std::random_device{}());
    std::uniform_real_distribution<double> base_dist(0.1, 10.0);
    std::uniform_real_distribution<double> exp_dist(0.0, 6.0);

    std::ofstream out(csv_name("pow"));
    write_header(out);

    for (int i = 0; i < n; ++i) {
        const double x = base_dist(gen);
        const double y = exp_dist(gen);
        const size_t id = server.add_task([x, y] {
            return ResultRow{0, "pow", x, y, std::pow(x, y)};
        });

        ResultRow row = server.request_result(id);
        row.task_id = id;
        write_row(out, row);
    }
}

int parse_n(int argc, char** argv) {
    int n = 1000;
    if (argc >= 2) {
        n = std::stoi(argv[1]);
    }
    if (n <= 5 || n >= 10000) {
        throw std::invalid_argument("N must satisfy 5 < N < 10000");
    }
    return n;
}

int main(int argc, char** argv) {
    try {
        const int n = parse_n(argc, argv);
        fs::create_directories("results");

        Server server;
        server.start();

        const auto t0 = std::chrono::steady_clock::now();

        std::thread c1(client_sin, std::ref(server), n);
        std::thread c2(client_sqrt, std::ref(server), n);
        std::thread c3(client_pow, std::ref(server), n);

        c1.join();
        c2.join();
        c3.join();

        server.stop();

        const auto t1 = std::chrono::steady_clock::now();
        const std::chrono::duration<double> elapsed = t1 - t0;

        std::cout << "Done. N per client: " << n << '\n'
                  << "Total tasks: " << 3 * n << '\n'
                  << "Elapsed: " << elapsed.count() << " s\n"
                  << "Files: results/client_sin.csv, results/client_sqrt.csv, results/client_pow.csv\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
