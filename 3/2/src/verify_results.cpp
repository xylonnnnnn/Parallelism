#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

struct Row {
    size_t task_id{};
    std::string operation;
    double x{};
    double y{};
    double result{};
};

std::vector<std::string> split_csv_line(const std::string& line) {
    std::vector<std::string> parts;
    std::stringstream ss(line);
    std::string item;
    while (std::getline(ss, item, ',')) {
        parts.push_back(item);
    }
    return parts;
}

Row parse_row(const std::string& line) {
    auto parts = split_csv_line(line);
    if (parts.size() != 5) {
        throw std::runtime_error("bad csv line: " + line);
    }
    return Row{static_cast<size_t>(std::stoull(parts[0])), parts[1],
               std::stod(parts[2]), std::stod(parts[3]), std::stod(parts[4])};
}

double expected_value(const Row& row) {
    if (row.operation == "sin") {
        return std::sin(row.x);
    }
    if (row.operation == "sqrt") {
        return std::sqrt(row.x);
    }
    if (row.operation == "pow") {
        return std::pow(row.x, row.y);
    }
    throw std::runtime_error("unknown operation: " + row.operation);
}

bool close_enough(double a, double b) {
    const double abs_err = std::abs(a - b);
    const double scale = std::max({1.0, std::abs(a), std::abs(b)});
    return abs_err <= 1e-10 * scale;
}

size_t verify_file(const fs::path& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("cannot open " + path.string());
    }

    std::string line;
    std::getline(in, line);
    if (line != "task_id,operation,x,y,result") {
        throw std::runtime_error("bad header in " + path.string());
    }

    size_t count = 0;
    while (std::getline(in, line)) {
        if (line.empty()) {
            continue;
        }
        Row row = parse_row(line);
        const double expected = expected_value(row);
        if (!close_enough(expected, row.result)) {
            std::ostringstream msg;
            msg << "check failed in " << path << " task_id=" << row.task_id
                << " expected=" << std::setprecision(17) << expected
                << " got=" << std::setprecision(17) << row.result;
            throw std::runtime_error(msg.str());
        }
        ++count;
    }
    return count;
}

int main() {
    try {
        const std::vector<fs::path> files = {
            "results/client_sin.csv",
            "results/client_sqrt.csv",
            "results/client_pow.csv"
        };

        size_t total = 0;
        for (const auto& file : files) {
            const size_t count = verify_file(file);
            std::cout << file.string() << ": OK, rows=" << count << '\n';
            total += count;
        }
        std::cout << "All checks passed. Total rows=" << total << '\n';
    } catch (const std::exception& e) {
        std::cerr << "Verification error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
