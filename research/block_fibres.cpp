// C++17 finite certificate for the unpadded n = m + 1 bound.
// Build with optimization. The largest histogram occupies 80,000,000 bytes.
#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

using Integer = std::int64_t;
constexpr std::array<int, 8> ten{1, 10, 100, 1000, 10000, 100000, 1000000, 10000000};

const auto pairs = [] {
    std::vector<std::pair<int, int>> result;
    for (int a = 2; a <= 9; ++a)
        for (int b = 2; b <= 9; ++b)
            if (a * b >= 10) result.emplace_back(a, b);
    return result;
}();

void require(bool condition) {
    if (!condition) throw std::runtime_error("certificate mismatch");
}

Integer fibre(int length, int a, int b) {
    // Append the units pair to a shorter block: r = a*u + b*v + 10*r_old.
    std::vector<Integer> old{1};
    for (int j = 1; j <= length; ++j) {
        const int q = ten[j];
        std::vector<Integer> next(q);
        for (auto [u, v] : pairs) {
            int r = (a*u + b*v) % q;
            for (auto count : old) {
                next[r] += count;
                r += 10;
                if (r >= q) r -= q;
            }
        }
        old.swap(next);
    }
    return *std::max_element(old.begin(), old.end());
}

Integer inverse(Integer a, Integer modulus) {
    Integer r = modulus, s = a % modulus, x = 0, y = 1;
    while (s) {
        const Integer quotient = r / s;
        const Integer remainder = r % s, next = x - quotient*y;
        r = s; s = remainder; x = y; y = next;
    }
    require(r == 1);
    return (x % modulus + modulus) % modulus;
}

struct Suffix { Integer y, b, carry; };

std::set<int> possible_delayed_slopes() {
    // The linked Python certificate proves H=2 when the agreement lasts
    // three digits. Seven input digits determine six digits after division.
    std::vector<Suffix> states{{0, 0, 0}};
    for (int k = 0; k < 7; ++k) {
        std::vector<Suffix> next;
        for (const auto& state : states) {
            for (auto [u, v] : pairs) {
                const auto y = state.y + ten[k]*u, b = state.b + ten[k]*v;
                const auto sum = state.carry + u*state.b + v*state.y;
                const int agreement = std::gcd(2000, ten[k + 1]);
                if ((y - b) % agreement == 0 && y % 2 == 0 && b % 2 == 0 && sum % 20 == 0)
                    next.push_back({y, b, sum/10});
            }
        }
        states.swap(next);
    }
    std::set<int> slopes;
    for (const auto& state : states) {
        require(std::gcd(state.b/2, Integer(10)) == 1);
        auto s = (state.y/2 * inverse(state.b/2, ten[6])) % ten[6];
        require(s % 1000 == 1 && s % 10000 != 1);
        slopes.insert(static_cast<int>(s));
    }
    require(slopes.size() == 126);
    return slopes;
}

int main() {
    Integer ordinary = 0, mixed = 0, delay2_small = 0, delay2_lift = 0;
    Integer delay3 = 0, final_lifts = 0;
    for (int s = 1; s < ten[4]; ++s)
        if (s % 10 && s % 100 != 1 && s != 2)
            ordinary = std::max(ordinary, fibre(4, 1, s));

    // CRT: for mixed coefficients, scale by a unit so that the even
    // coefficient is 1 mod 625 and the multiple of five is 1 mod 16.
    for (int a = 1; a < ten[4]; a += 625)
        if (a % 2 == 0)
            for (int b = 1; b < ten[4]; b += 16)
                if (b % 5 == 0) mixed = std::max(mixed, fibre(4, a, b));

    int hard2 = 0;
    for (int s = 101; s < ten[5]; s += 100) {
        if (s % 1000 == 1) continue;
        const auto size = fibre(5, 1, s);
        if (size <= 16000) {
            delay2_small = std::max(delay2_small, size);
        } else {
            ++hard2;
            for (int digit = 0; digit <= 9; ++digit)
                delay2_lift = std::max(delay2_lift, fibre(6, 1, s + digit*ten[5]));
        }
    }

    int hard = 0;
    for (int s : possible_delayed_slopes()) {
        const auto size = fibre(6, 1, s);
        delay3 = std::max(delay3, size);
        if (size <= 112696) continue; // floor(6.95^6)
        ++hard;
        for (int digit = 0; digit <= 9; ++digit)
            final_lifts = std::max(final_lifts, fibre(7, 1, s + digit*ten[6]));
    }
    require(ordinary == 2332 && mixed == 2199);
    require(delay2_small == 15402 && hard2 == 4 && delay2_lift == 108536);
    require(delay3 == 118784 && hard == 44 && final_lifts == 693568);
    std::cout << "ordinary " << ordinary << ", mixed " << mixed
              << ", delay two: " << delay2_small << " or " << delay2_lift
              << ", delay three peak " << delay3 << ", hard slopes " << hard
              << ", seventh-digit lifts " << final_lifts << '\n';
}
