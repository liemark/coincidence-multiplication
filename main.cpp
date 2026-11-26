#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <limits>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>

namespace {

using BigInt = boost::multiprecision::cpp_int;

struct Digits {
    int y;
    int b;
};

using DigitChoices = std::array<std::vector<Digits>, 10>;

// The next column depends on the units digits and the carry modulo 10.
const auto digit_choices = [] {
    std::array<std::array<DigitChoices, 10>, 10> table;
    for (int y_units = 0; y_units < 10; ++y_units) {
        for (int b_units = 0; b_units < 10; ++b_units) {
            for (int b = 9; b >= 0; --b) {
                for (int y = 0; y < 10; ++y) {
                    const int residue = (10 - (y * b_units + b * y_units) % 10) % 10;
                    table[y_units][b_units][residue].push_back({y, b});
                }
            }
        }
    }
    return table;
}();

struct Factors {
    std::string a;
    std::string b;
};

struct Results {
    BigInt padded_count = 0;
    std::vector<Factors> padded;
    std::vector<Factors> unpadded;

    void append(Results&& other, std::size_t display_limit) {
        padded_count += other.padded_count;
        const auto count = std::min(display_limit - padded.size(), other.padded.size());
        padded.insert(padded.end(), std::make_move_iterator(other.padded.begin()),
                      std::make_move_iterator(other.padded.begin() + count));
        unpadded.insert(unpadded.end(), std::make_move_iterator(other.unpadded.begin()),
                        std::make_move_iterator(other.unpadded.end()));
    }
};

std::string decimal(std::uint64_t value) {
    return std::to_string(value);
}

std::string decimal(const BigInt& value) {
    return value.str();
}

template <typename Integer>
class PaddedSearch {
public:
    PaddedSearch(int digits, int prefix_digits, int trailing_zeros, std::size_t display_limit)
        : digits_(digits),
          powers_(static_cast<std::size_t>(digits) + 1, 1),
          trailing_zeros_(trailing_zeros, '0'),
          display_limit_(display_limit) {
        for (int k = 1; k <= digits_; ++k) {
            powers_[k] = powers_[k - 1] * 10;
        }
        min_prefix_ = powers_[prefix_digits - 1];
        max_prefix_ = powers_[prefix_digits];

        // For q = 10^m, all cross terms are maximized when both suffixes
        // are all nines. Their sum is 2(q - 1)(q - 10)/11.
        // The carry is that sum divided by q, so x * gap <= floor((2q - 22)/11).
        max_gap_ = (2 * powers_.back() - 22) / 11 / min_prefix_;
    }

    void run(int requested_threads, Results& results) const {
        struct Job {
            Digits units;
            Results results;
        };
        std::vector<Job> jobs;
        for (int b = 9; b >= 0 && Integer(10 - b) <= max_gap_; --b) {
            for (int y = 0; y < 10; ++y) {
                if (y != 0 || b != 0) {
                    jobs.push_back({{y, b}, {}});
                }
            }
        }

        const auto worker_count = std::min(static_cast<std::size_t>(requested_threads), jobs.size());
        std::atomic<std::size_t> next_job{0};
        const auto worker = [&] {
            for (auto index = next_job.fetch_add(1, std::memory_order_relaxed);
                 index < jobs.size();
                 index = next_job.fetch_add(1, std::memory_order_relaxed)) {
                auto& job = jobs[index];
                const auto [y, b] = job.units;
                search(1, Integer(y), Integer(10 - b), Integer(0),
                       trailing_zeros_.empty() && y * b >= 10,
                       digit_choices[y][b], job.results);
            }
        };

        std::vector<std::thread> threads;
        threads.reserve(worker_count);
        for (std::size_t i = 1; i < worker_count; ++i) {
            try {
                threads.emplace_back(worker);
            } catch (const std::system_error&) {
                // The existing workers can finish the same queue with fewer threads.
                break;
            }
        }
        worker();
        for (auto& thread : threads) {
            thread.join();
        }
        for (auto& job : jobs) {
            results.append(std::move(job.results), display_limit_);
        }
    }

private:
    void search(int k, const Integer& y, const Integer& gap, const Integer& carry,
                bool unpadded, const DigitChoices& choices, Results& results) const {
        if (k == digits_) {
            finish(y, gap, carry, unpadded, results);
            return;
        }

        // gap is the smallest possible 10^m - B for the chosen low digits of B.
        const Integer& place = powers_[k];
        const Integer b = place - gap;
        const auto residue = static_cast<unsigned>(carry % 10);
        for (const auto [y_digit, b_digit] : choices[residue]) {
            const Integer next_gap = gap + (9 - b_digit) * place;
            if (next_gap > max_gap_) {
                break;
            }

            // The diagonal product cancels. Only the new cross terms change the carry.
            const Integer next_carry = (carry + y_digit * b + b_digit * y) / 10;
            const Integer next_y = y + y_digit * place;
            search(k + 1, next_y, next_gap, next_carry,
                   unpadded && y_digit * b_digit >= 10, choices, results);
        }
    }

    void finish(const Integer& y, const Integer& gap, const Integer& carry,
                bool unpadded, Results& results) const {
        // The fixed-width equality is carry = x * (10^m - B).
        if (carry < min_prefix_ * gap || carry % gap != 0) {
            return;
        }
        const Integer prefix = carry / gap;
        if (prefix >= max_prefix_) {
            return;
        }

        ++results.padded_count;
        const bool show_padded = results.padded.size() < display_limit_;
        if (show_padded || unpadded) {
            std::string suffix = decimal(y);
            suffix.insert(0, digits_ - suffix.size(), '0');
            const Integer b = powers_.back() - gap;
            Factors factors{decimal(prefix) + suffix + trailing_zeros_,
                            decimal(b) + trailing_zeros_};
            if (unpadded) {
                results.unpadded.push_back(factors);
            }
            if (show_padded) {
                results.padded.push_back(std::move(factors));
            }
        }
    }

    int digits_;
    std::vector<Integer> powers_;
    std::string trailing_zeros_;
    std::size_t display_limit_;
    Integer min_prefix_, max_prefix_, max_gap_;
};

BigInt power_of_ten(int exponent) {
    BigInt result = 1;
    BigInt base = 10;
    while (exponent > 0) {
        if (exponent % 2 != 0) {
            result *= base;
        }
        exponent /= 2;
        if (exponent > 0) {
            base *= base;
        }
    }
    return result;
}

void print_factors(const std::vector<Factors>& factors) {
    for (const auto& [a, b] : factors) {
        std::cout << "A = " << a << ", B = " << b << '\n';
    }
}

} // namespace

int main(int argc, char* argv[]) {
    const bool show_all = argc == 2 && std::string_view(argv[1]) == "--all";
    if (argc != 1 && !show_all) {
        std::cerr << "Usage: " << argv[0] << " [--all]\n";
        return 1;
    }

    int n, m, num_threads;
    std::cout << "Please enter the number of digits for the first factor (n): ";
    if (!(std::cin >> n)) {
        std::cerr << "Expected an integer for n.\n";
        return 1;
    }
    std::cout << "Please enter the number of digits for the second factor (m, n > m): ";
    if (!(std::cin >> m) || m < 1 || n <= m) {
        std::cerr << "Expected digit counts with n > m >= 1.\n";
        return 1;
    }
    std::cout << "Please enter the number of threads to use: ";
    if (!(std::cin >> num_threads)) {
        std::cerr << "Expected an integer for the thread count.\n";
        return 1;
    }
    num_threads = std::max(1, num_threads);

    const auto display_limit = show_all ? std::numeric_limits<std::size_t>::max() : 1000;
    const int prefix_digits = n - m;
    const auto start = std::chrono::steady_clock::now();
    Results results;
    std::cout << "\nSearching for n=" << n << ", m=" << m << "..." << std::endl;
    // A common trailing zero can be removed from both factors and adds "00" to
    // the fixed-width product when restored. Cross terms vanish for m = 1.
    for (int digits = m; digits >= std::max(2, prefix_digits); --digits) {
        // Each carry update is smaller than 19 * 10^(digits - 1).
        if (digits <= 18) {
            PaddedSearch<std::uint64_t>(digits, prefix_digits, m - digits, display_limit)
                .run(num_threads, results);
        } else {
            PaddedSearch<BigInt>(digits, prefix_digits, m - digits, display_limit)
                .run(num_threads, results);
        }
    }
    const std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - start;

    std::cout << "\nFixed-width solutions";
    if (results.padded_count > results.padded.size()) {
        std::cout << " (first " << results.padded.size() << ')';
    }
    std::cout << ":\n";
    print_factors(results.padded);
    std::cout << "Fixed-width total: " << results.padded_count << '\n';

    std::cout << "\nSolutions without zero padding:\n";
    print_factors(results.unpadded);
    BigInt unpadded_count = results.unpadded.size();
    // A shorter suffix can only match when m = 1 and B = 1.
    if (m == 1) {
        const BigInt first = power_of_ten(n - 1);
        const BigInt last = first * 10 - 1;
        unpadded_count = last - first + 1;
        if (show_all) {
            for (BigInt a = first; a <= last; ++a) {
                std::cout << "A = " << a << ", B = 1\n";
            }
        } else {
            std::cout << "A in [" << first << ", " << last << "], B = 1\n";
        }
    }
    std::cout << "No-padding total: " << unpadded_count << '\n';
    std::cout << "\nSearch time: " << elapsed.count() << " seconds.\n";
    std::cout << "\nPress Enter to exit..." << std::endl;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
}
