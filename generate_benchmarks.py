#!/usr/bin/env python3
import re
import os

# Read master_benchmark.cpp
with open('tests/master_benchmark.cpp', 'r') as f:
    content = f.read()

# Find all BENCH and BENCH_BITNFA calls
pattern = r'BENCH(?:_BITNFA)?\s*\(\s*"([^"]+)"\s*,\s*"([^"]+)"\s*,\s*([^,]+?),\s*"([^"]+)"\s*\)'
matches = re.findall(pattern, content)

print(f"Found {len(matches)} benchmark patterns")

# Create output directory
os.makedirs('tests/individual_benchmarks', exist_ok=True)

for match in matches:
    name, regex_pattern, input_expr, desc = match
    
    # Check if this uses BitNFA
    # Search for the full line to determine if it's BENCH_BITNFA
    line_pattern = rf'BENCH(?:_BITNFA)?\s*\(\s*"{re.escape(name)}"'
    for line in content.split('\n'):
        if re.search(line_pattern, line):
            use_bitnfa = 'BENCH_BITNFA' in line
            break
    
    bitnfa_include = '#include <ctre/bitnfa/integration.hpp>' if use_bitnfa else ''
    bitnfa_call = f'ctre::bitnfa::match_auto<"{regex_pattern}">' if use_bitnfa else f'ctre::match<"{regex_pattern}">'
    
    cpp_code = f'''#include <chrono>
#include <iostream>
#include <string>
#include <ctre.hpp>
{bitnfa_include}

inline std::string gen_repeat(char c, size_t len) {{
    return std::string(len, c);
}}

inline std::string gen_range(char start, size_t count, size_t len) {{
    std::string result;
    for (size_t i = 0; i < len; ++i)
        result += static_cast<char>(start + (i % count));
    return result;
}}

inline std::string gen_sparse(const char* chars, size_t char_count, size_t len) {{
    std::string result;
    for (size_t i = 0; i < len; ++i)
        result += chars[i % char_count];
    return result;
}}

int main() {{
    const int ITERATIONS = 100000;
    std::string test_input = {input_expr};
    
    // Warmup
    for (int i = 0; i < 10000; ++i) {{
        volatile bool r = static_cast<bool>({bitnfa_call}(test_input));
    }}
    
    // Benchmark
    double min_time = 1e9;
    for (int sample = 0; sample < 10; ++sample) {{
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < ITERATIONS; ++i) {{
            volatile bool r = static_cast<bool>({bitnfa_call}(test_input));
        }}
        auto end = std::chrono::high_resolution_clock::now();
        double time_ns = std::chrono::duration<double, std::nano>(end - start).count() / ITERATIONS;
        if (time_ns < min_time) min_time = time_ns;
    }}
    
    std::cout << "{name}|{regex_pattern}|" << min_time << "|{desc}" << std::endl;
    return 0;
}}
'''
    
    with open(f'tests/individual_benchmarks/{name}_bench.cpp', 'w') as f:
        f.write(cpp_code)

print(f"Generated {len(matches)} benchmark files in tests/individual_benchmarks/")
