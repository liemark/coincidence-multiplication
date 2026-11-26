// Verification of the n = m + 1, m = 2..12 unpadded counts.
// Build as C++17 with optimization. OpenMP is optional.
#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

using Integer = std::int64_t;
using Pair = std::pair<int, int>;
using Choices = std::array<std::vector<Pair>, 10>;
constexpr int max_digits = 12;

const auto ten = [] {
    std::array<Integer, max_digits + 1> powers{1};
    for (int i = 1; i <= max_digits; ++i) powers[i] = 10 * powers[i-1];
    return powers;
}();

const auto digit_choices = [] {
    std::array<std::array<Choices, 10>, 10> choices;
    for (int a = 2; a <= 9; ++a)
        for (int b = 2; b <= 9; ++b)
            for (int u = 2; u <= 9; ++u)
                for (int v = 2; v <= 9; ++v)
                    if (u*v >= 10)
                        choices[a][b][(10 - (u*b + v*a)%10)%10].emplace_back(u, v);
    return choices;
}();

struct Result {
    Integer leaves = 0;
    std::vector<std::pair<Integer, Integer>> solutions;
};

void search(int k, int m, Integer a, Integer b, Integer carry,
            const Choices& choices, Result& out) {
    if (k == m) {
        ++out.leaves;
        const Integer gap = ten[m] - b;
        const Integer x = carry / gap;
        if (carry % gap == 0 && x >= 1 && x <= 9)
            out.solutions.emplace_back(x*ten[m]+a, b);
        return;
    }
    for (auto [u, v] : choices[carry % 10]) {
        if (k+1 == m && v < 8) continue; // B > (9/11)*10^m
        search(k+1, m, a+u*ten[k], b+v*ten[k], (carry+u*b+v*a)/10, choices, out);
    }
}

int main(int argc, char** argv) {
    const bool list = argc == 2 && std::string_view(argv[1]) == "--list";
    if (argc != 1 && !list) {
        std::cerr << "Usage: exact_unpadded_counts [--list]\n";
        return 1;
    }
    constexpr int expected[] = {0, 0, 20, 8, 4, 2, 2, 2, 2, 0, 0, 0, 0};
    for (int m = 2; m <= max_digits; ++m) {
        std::array<Result, 100> results;
#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic, 1) num_threads(8)
#endif
        for (int job = 0; job < 100; ++job) {
            const int a = job/10, b = job%10;
            if (a*b >= 10) search(1, m, a, b, 0, digit_choices[a][b], results[job]);
        }
        Integer leaves = 0;
        std::size_t count = 0;
        for (const auto& result : results) {
            leaves += result.leaves;
            count += result.solutions.size();
        }
        if (count != static_cast<std::size_t>(expected[m])) throw std::runtime_error("count mismatch");
        std::cout << "m=" << m << " leaves=" << leaves << " solutions=" << count << std::endl;
        if (list) {
            for (const auto& result : results)
                for (const auto& [a, b] : result.solutions)
                    std::cout << "A=" << a << " B=" << b << '\n';
        }
    }
}
