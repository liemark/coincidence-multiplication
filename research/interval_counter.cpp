// Independent high-to-low enumeration for n=m+1. No carry or B-prefix bound.
// Arithmetic is on high-prefix residuals; ordinary products need not fit uint64_t.
#include <cstdint>
#include <iostream>
#include <string_view>
#include <utility>
#include <vector>

using Factor = std::uint64_t;
using Solution = std::pair<Factor, Factor>;

struct DigitPair {
    int a, b;
};

const auto digits = [] {
    std::vector<DigitPair> result;
    for (int a = 2; a <= 9; ++a)
        for (int b = 2; b <= 9; ++b)
            if (a*b >= 10) result.push_back({a, b});
    return result;
}();

void visit(Factor a, Factor b, Factor delta, int remaining, std::vector<Solution>& solutions) {
    // delta = C - a*b, where C is the constructed product prefix.
    // For any extension 0 <= delta < (a+1)(b+1)-a*b = a+b+1.
    if (delta > a+b) return;
    // In the final column the residual must be zero.
    const Factor slack = 10*delta;
    if (remaining == 1) {
        for (int u = 2; u <= 9; ++u) {
            if (b*u >= slack) break;
            const Factor numerator = slack-b*u;
            const auto v = numerator/a;
            if (v >= 2 && v <= 9 && u*v >= 10 && v*a == numerator)
                solutions.emplace_back(10*a+u, 10*b+v);
        }
        return;
    }
    for (const auto& digit : digits) {
        const Factor cross = b*digit.a + a*digit.b;
        if (cross <= slack)
            visit(10*a+digit.a, 10*b+digit.b, 10*(slack-cross), remaining-1, solutions);
    }
}

int main(int argc, char** argv) {
    const bool list = argc == 2 && std::string_view(argv[1]) == "--list";
    if (argc != 1 && !list) {
        std::cerr << "Usage: interval_counter [--list]\n";
        return 1;
    }
    for (int m = 2; m <= 12; ++m) {
        // Every allowed pair of leading digits is tried for all nine prefixes.
        std::vector<std::vector<Solution>> jobs(9*digits.size());
        const int job_count = static_cast<int>(jobs.size());
#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic, 1) num_threads(8)
#endif
        for (int i = 0; i < job_count; ++i) {
            const auto& d = digits[i % digits.size()];
            const int x = 1 + i/static_cast<int>(digits.size());
            visit(10*x+d.a, d.b, 10*x*(10-d.b), m-1, jobs[i]);
        }
        std::size_t count = 0;
        for (const auto& result : jobs) count += result.size();
        std::cout << "m=" << m << " solutions=" << count << std::endl;
        if (list)
            for (const auto& result : jobs)
                for (auto [a, b] : result) std::cout << "A=" << a << " B=" << b << '\n';
    }
}
