#include <ctre.hpp>
#include <iostream>
#include <chrono>
#include <string>

// Include analysis ONLY in this file
#include <ctre/do_analysis_simple.hpp>

// Custom match that uses prefiltering
template <ctll::fixed_string Pattern>
auto match_with_prefilter(std::string_view input) {
    using tmp = typename ctll::parser<ctre::pcre, Pattern, ctre::pcre_actions>::template output<ctre::pcre_context<>>;
    using AST = decltype(ctll::front(typename tmp::output_type::stack_type()));
    using RE = AST;
    
    constexpr bool has_lit = ctre::prefilter::pattern_has_literal<RE>();
    constexpr size_t lit_len = ctre::prefilter::pattern_literal_length<RE>();
    
    if constexpr (has_lit && lit_len >= 2) {
        bool found = ctre::prefilter::scan_for_pattern_literal<RE>(
            input.data(), input.data() + input.size(),
            std::make_index_sequence<lit_len>{}
        );
        
        if (!found) {
            // Fail-fast
            return ctre::regex_results<const char*>{};
        }
    }
    
    // Normal match
    return ctre::match<Pattern>(input);
}

template<typename Func>
double bench(Func&& f, int iter = 100000) {
    volatile bool dummy = false;
    for (int w = 0; w < 10000; ++w) dummy = f();
    
    double min_time = 1e9;
    for (int sample = 0; sample < 10; ++sample) {
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < iter; ++i) {
            dummy = f();
        }
        auto end = std::chrono::high_resolution_clock::now();
        double t = std::chrono::duration<double, std::nano>(end - start).count() / iter;
        if (t < min_time) min_time = t;
    }
    return min_time;
}

int main() {
    std::cout << "Testing direct integration...\n\n";
    
    // Pattern without literal
    std::string inp1(32, 'a');
    double t1 = bench([&]() { return ctre::match<"a+">(inp1); });
    double t2 = bench([&]() { return match_with_prefilter<"a+">(inp1); });
    
    std::cout << "a+_32:\n";
    std::cout << "  Standard:  " << t1 << " ns\n";
    std::cout << "  Prefilter: " << t2 << " ns\n";
    std::cout << "  Overhead:  " << ((t2/t1 - 1) * 100) << "%\n\n";
    
    // Pattern WITH literal, no match
    std::string inp2(100, 'x');
    double t3 = bench([&]() { return ctre::match<"(foo|bar)test">(inp2); }, 50000);
    double t4 = bench([&]() { return match_with_prefilter<"(foo|bar)test">(inp2); }, 50000);
    
    std::cout << "(foo|bar)test (no match, 100 bytes):\n";
    std::cout << "  Standard:  " << t3 << " ns\n";
    std::cout << "  Prefilter: " << t4 << " ns\n";
    std::cout << "  Speedup:   " << (t3/t4) << "x\n";
    
    return 0;
}
