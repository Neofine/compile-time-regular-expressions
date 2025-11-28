#!/bin/bash

# Extract patterns from master_benchmark.cpp and generate individual benchmark files
cd /root/compile-time-regular-expressions

# Create output directory
mkdir -p tests/individual_benchmarks

# Parse master_benchmark.cpp to extract BENCH() and BENCH_BITNFA() calls
grep -E "BENCH\(|BENCH_BITNFA\(" tests/master_benchmark.cpp | while IFS= read -r line; do
    # Extract pattern details
    if [[ $line =~ BENCH[_BITNFA]*\(\"([^\"]+)\",\ *\"([^\"]+)\",\ *([^,]+),\ *\"([^\"]+)\" ]]; then
        name="${BASH_REMATCH[1]}"
        pattern="${BASH_REMATCH[2]}"
        input_expr="${BASH_REMATCH[3]}"
        desc="${BASH_REMATCH[4]}"
        
        # Determine if it uses BitNFA
        use_bitnfa="false"
        if [[ $line =~ BENCH_BITNFA ]]; then
            use_bitnfa="true"
        fi
        
        echo "Generating benchmark for: $name"
        
        # Create individual benchmark file
        cat > "tests/individual_benchmarks/${name}_bench.cpp" << BENCH_EOF
#include <chrono>
#include <iostream>
#include <string>
#include <ctre.hpp>
$([ "$use_bitnfa" = "true" ] && echo "#include <ctre/bitnfa/integration.hpp>")

// Helper functions from master_benchmark.cpp
inline std::string gen_repeat(char c, size_t len) {
    return std::string(len, c);
}

inline std::string gen_range(char start, size_t count, size_t len) {
    std::string result;
    for (size_t i = 0; i < len; ++i)
        result += static_cast<char>(start + (i % count));
    return result;
}

inline std::string gen_sparse(const char* chars, size_t char_count, size_t len) {
    std::string result;
    for (size_t i = 0; i < len; ++i)
        result += chars[i % char_count];
    return result;
}

int main() {
    const int ITERATIONS = 100000;
    std::string test_input = $input_expr;
    
    // Warmup
    for (int i = 0; i < 10000; ++i) {
$([ "$use_bitnfa" = "true" ] && echo "        volatile bool r = static_cast<bool>(ctre::bitnfa::match_auto<\"$pattern\">(test_input));" || echo "        volatile bool r = static_cast<bool>(ctre::match<\"$pattern\">(test_input));")
    }
    
    // Benchmark
    double min_time = 1e9;
    for (int sample = 0; sample < 10; ++sample) {
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < ITERATIONS; ++i) {
$([ "$use_bitnfa" = "true" ] && echo "            volatile bool r = static_cast<bool>(ctre::bitnfa::match_auto<\"$pattern\">(test_input));" || echo "            volatile bool r = static_cast<bool>(ctre::match<\"$pattern\">(test_input));")
        }
        auto end = std::chrono::high_resolution_clock::now();
        double time_ns = std::chrono::duration<double, std::nano>(end - start).count() / ITERATIONS;
        if (time_ns < min_time) min_time = time_ns;
    }
    
    std::cout << "$name|$pattern|" << min_time << "|$desc" << std::endl;
    return 0;
}
BENCH_EOF
    fi
done

echo "Generated individual benchmark files in tests/individual_benchmarks/"
