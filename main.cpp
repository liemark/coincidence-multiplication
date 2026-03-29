#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <algorithm>
#include <stdexcept>
#include <thread>
#include <mutex>
#include <functional>
#include <sstream>
#include <cmath>

using int_type = long long; // Use long long for speed

struct Context {
    int n, m;
    int_type x_min, x_max;
    int_type power_of_10_m;
    int_type power_of_10_2m;
    std::vector<int_type> powers_of_10;
    std::vector<std::pair<int_type, int_type>> found_pairs; // Each thread will have its own result list
};

void _solve(int k, int_type y, int_type b, int_type carry, Context& context);
void _final_verification(int_type y_val, int_type b_val, Context& context);

// Function to find primitive solutions in parallel
std::vector<std::pair<int_type, int_type>> find_primitive_solutions_parallel(int n, int m, int num_threads) {
    if (n <= m || m <= 0) return {};
    std::cout << "\nParallel searching for primitive solutions where n=" << n << ", m=" << m << " (using " << num_threads << " threads)..." << std::endl;

    std::vector<std::thread> threads;
    std::vector<std::pair<int_type, int_type>> final_results;
    std::mutex results_mutex;

    // Distribute tasks to threads (based on the unit digit of b: 0-9)
    std::vector<std::vector<int>> work_distribution(num_threads);
    for (int i = 0; i <= 9; ++i) {
        work_distribution[i % num_threads].push_back(i);
    }

    // Worker function for each thread
    auto worker = [&](const std::vector<int>& assigned_b_digits) {
        Context context;
        context.n = n; context.m = m;
        context.x_min = (n > m) ? static_cast<int_type>(pow(10, n - m - 1)) : 0;
        context.x_max = static_cast<int_type>(pow(10, n - m));
        context.power_of_10_m = static_cast<int_type>(pow(10, m));
        context.power_of_10_2m = static_cast<int_type>(pow(10, 2 * m));
        for (int i = 0; i <= 2 * m; ++i) {
            context.powers_of_10.push_back(static_cast<int_type>(pow(10, i)));
        }

        // Thread only processes its assigned b_digit
        for (int b_digit : assigned_b_digits) {
            for (int y_digit = 0; y_digit <= 9; ++y_digit) {
                if (y_digit == 0 && b_digit == 0) continue;
                if (y_digit * b_digit >= 100) continue;
                // The term for k=0 is always 0, so directly proceed to the next recursive layer
                _solve(1, y_digit, b_digit, 0, context);
            }
        }

        // Add thread's results to the final results
        std::lock_guard<std::mutex> lock(results_mutex);
        final_results.insert(final_results.end(), context.found_pairs.begin(), context.found_pairs.end());
        };

    // Launch threads
    for (int i = 0; i < num_threads; ++i) {
        if (!work_distribution[i].empty()) {
            threads.emplace_back(worker, work_distribution[i]);
        }
    }

    // Wait for all threads to finish
    for (auto& t : threads) {
        t.join();
    }

    std::cout << "Found " << final_results.size() << " sets of primitive solutions." << std::endl;
    return final_results;
}


void _solve(int k, int_type y, int_type b, int_type carry, Context& context) {
    if (k == context.m) {
        _final_verification(y, b, context);
        return;
    }
    int_type power_of_10_k = context.powers_of_10[k];
    for (int b_digit = 0; b_digit <= 9; ++b_digit) {
        if (k == context.m - 1 && b_digit == 0) continue;
        for (int y_digit = 0; y_digit <= 9; ++y_digit) {
            if (y_digit * b_digit >= 100) continue;
            int_type term = carry + y_digit * b + b_digit * y;
            if (term % 10 == 0) {
                _solve(k + 1, y + y_digit * power_of_10_k, b + b_digit * power_of_10_k, term / 10, context);
            }
        }
    }
}

void _final_verification(int_type y_val, int_type b_val, Context& context) {
    int m = context.m;
    int_type denominator = context.power_of_10_2m - (b_val * context.power_of_10_m);
    if (denominator <= 0) return; // Denominator check

    // Range estimation check
    int_type temp_b = b_val;
    int_type p_low_max_for_b = 0;
    for (int i = 0; i < m; ++i) {
        p_low_max_for_b += 9 * (temp_b % 10) * context.powers_of_10[2 * i];
        temp_b /= 10;
    }

    int_type y_min_bound = 0;
    int_type y_max_bound = context.power_of_10_m;
    if (b_val != 0) {
        y_min_bound = (context.x_min * denominator) / b_val;
        y_max_bound = (context.x_max * denominator + p_low_max_for_b) / b_val;
    }

    if (y_val < y_min_bound || y_val > y_max_bound) return;


    int_type p_low = 0;
    int_type temp_y_p = y_val;
    int_type temp_b_p = b_val;
    for (int i = 0; i < m; ++i) {
        int_type y_digit = (temp_y_p / context.powers_of_10[m - 1 - i]) % 10;
        int_type b_digit = (temp_b_p / context.powers_of_10[m - 1 - i]) % 10;
        p_low += y_digit * b_digit * context.powers_of_10[2 * (m - 1 - i)];
    }

    int_type numerator = y_val * b_val - p_low;
    if (numerator > 0 && numerator % denominator == 0) {
        int_type x_val = numerator / denominator;
        if (x_val >= context.x_min && x_val < context.x_max) {
            context.found_pairs.push_back({ x_val * context.power_of_10_m + y_val, b_val });
        }
    }
}


int main() {
    int n_target, m_target, num_threads;

    std::cout << "Please enter the number of digits for the first factor (n): ";
    std::cin >> n_target;
    std::cout << "Please enter the number of digits for the second factor (m, n > m): ";
    std::cin >> m_target;
    std::cout << "Please enter the number of threads to use: ";
    std::cin >> num_threads;

    if (num_threads <= 0) {
        num_threads = 1;
    }

    std::cout << "\nStarting to compute all coincidence multiplication solutions for n=" << n_target << ", m=" << m_target << std::endl;
    auto start_time = std::chrono::high_resolution_clock::now();

    std::vector<std::pair<int_type, int_type>> all_solutions;

    for (int m_current = m_target; m_current >= 1; --m_current) {
        int n_current = n_target - (m_target - m_current);
        if (n_current <= m_current) break;

        std::vector<std::pair<int_type, int_type>> primitives = find_primitive_solutions_parallel(n_current, m_current, num_threads);

        int_type scale_factor = 1;
        if (m_target - m_current > 0) {
            scale_factor = static_cast<int_type>(pow(10, m_target - m_current));
        }

        for (size_t i = 0; i < primitives.size(); ++i) {
            all_solutions.push_back({ primitives[i].first * scale_factor, primitives[i].second * scale_factor });
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;

    std::cout << "\nDisplaying partial solutions" << std::endl;
    size_t num_to_print = 1000;
    for (size_t i = 0; i < std::min(all_solutions.size(), num_to_print); ++i) {
        std::cout << "A = " << all_solutions[i].first << ", B = " << all_solutions[i].second << std::endl;
    }

    // Check if the solutions satisfy the no-zero-padding rule
    std::cout << "\n\nSolutions that satisfy the no-zero-padding rule:" << std::endl;
    int dual_rule_count = 0;
    for (size_t i = 0; i < all_solutions.size(); ++i) {
        int_type a = all_solutions[i].first;
        int_type b = all_solutions[i].second;

        int_type true_product = a * b;

        // Convert numbers to strings to process each digit
        std::stringstream ss_a, ss_b;
        ss_a << a;
        ss_b << b;
        std::string s_a = ss_a.str();
        std::string s_b = ss_b.str();

        // Pad with leading zeros to match target digit counts
        if (s_a.length() < (size_t)n_target) s_a.insert(0, n_target - s_a.length(), '0');
        if (s_b.length() < (size_t)m_target) s_b.insert(0, m_target - s_b.length(), '0');


        // Construct the coincidence product string according to the new rule
        std::stringstream ss_pc_new;
        if (n_target > m_target) {
            ss_pc_new << s_a.substr(0, n_target - m_target);
        }

        for (int j = 0; j < m_target; ++j) {
            int y_digit = s_a[n_target - m_target + j] - '0';
            int b_digit = s_b[j] - '0';
            int prod = y_digit * b_digit;
            ss_pc_new << prod; // No zero padding
        }
        std::string s_pc_new_str = ss_pc_new.str();

        // Convert the new coincidence product string to a numerical value
        int_type pc_val_new;
        std::stringstream ss_pc_val(s_pc_new_str);
        ss_pc_val >> pc_val_new;

        // Check for equality
        if (true_product == pc_val_new) {
            dual_rule_count++;
            std::cout << "    A = " << a << ", B = " << b << std::endl;
        }
    }
    std::cout << "Verification completed. Found a total of " << dual_rule_count << " special solutions." << std::endl;

    std::cout << "\n\nSearch completed." << std::endl;
    std::cout << "Total time: " << elapsed.count() << " seconds." << std::endl;
    std::cout << "Found a total of " << all_solutions.size() << " solution groups." << std::endl;

    std::cout << "\nPress Enter to exit..." << std::endl;
    std::cin.ignore();
    std::cin.get();

    return 0;
}
