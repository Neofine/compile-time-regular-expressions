#!/bin/bash

cd "$(dirname "$0")"

echo "Compiling and running individual benchmarks..."
echo "Each binary will be small (< 32KB) to avoid I-cache issues"
echo ""

# Create results directory
mkdir -p results/individual
rm -f results/individual/all_results.txt

# Define benchmark patterns: name|pattern|input_generator|description
BENCHMARKS=(
    # Character class ranges
    "[0-9]*_32|[0-9]*|gen_range('0',10,32)|Digit class star 32b"
    "[0-9]+_32|[0-9]+|gen_range('0',10,32)|Digit class plus 32b"
    "[0-9]*_256|[0-9]*|gen_range('0',10,256)|Digit class star 256b"
    "[0-9]+_256|[0-9]+|gen_range('0',10,256)|Digit class plus 256b"
    "[a-z]*_32|[a-z]*|gen_range('a',26,32)|Lower alpha star 32b"
    "[a-z]+_32|[a-z]+|gen_range('a',26,32)|Lower alpha plus 32b"
    "[a-z]*_128|[a-z]*|gen_range('a',26,128)|Lower alpha star 128b"
    "[a-z]+_128|[a-z]+|gen_range('a',26,128)|Lower alpha plus 128b"
    "[a-z]*_256|[a-z]*|gen_range('a',26,256)|Lower alpha star 256b"
    "[a-z]+_256|[a-z]+|gen_range('a',26,256)|Lower alpha plus 256b"
    "[a-z]*_512|[a-z]*|gen_range('a',26,512)|Lower alpha star 512b"
    "[a-z]+_512|[a-z]+|gen_range('a',26,512)|Lower alpha plus 512b"
    "[A-Z]*_32|[A-Z]*|gen_range('A',26,32)|Upper alpha star 32b"
    "[A-Z]+_32|[A-Z]+|gen_range('A',26,32)|Upper alpha plus 32b"
    "[A-Z]*_256|[A-Z]*|gen_range('A',26,256)|Upper alpha star 256b"
    
    # Multi-range character classes
    "[a-zA-Z]*_32|[a-zA-Z]*|gen_range('a',26,32)|Mixed case star 32b"
    "[a-zA-Z]+_32|[a-zA-Z]+|gen_range('a',26,32)|Mixed case plus 32b"
    "[a-zA-Z]*_64|[a-zA-Z]*|gen_range('a',26,64)|Mixed case star 64b"
    "[a-zA-Z]+_64|[a-zA-Z]+|gen_range('a',26,64)|Mixed case plus 64b"
    "[a-zA-Z]*_128|[a-zA-Z]*|gen_range('a',26,128)|Mixed case star 128b"
    "[0-9a-f]*_32|[0-9a-f]*|gen_range('0',16,32)|Hex lower star 32b"
    "[0-9a-f]+_32|[0-9a-f]+|gen_range('0',16,32)|Hex lower plus 32b"
    "[0-9a-f]*_64|[0-9a-f]*|gen_range('0',16,64)|Hex lower star 64b"
    "[0-9a-fA-F]*_32|[0-9a-fA-F]*|gen_range('0',16,32)|Hex mixed star 32b"
    "[0-9a-fA-F]+_32|[0-9a-fA-F]+|gen_range('0',16,32)|Hex mixed plus 32b"
    
    # Single character repetitions
    "a*_32|a*|gen_repeat('a',32)|Single char star 32b"
    "a+_32|a+|gen_repeat('a',32)|Single char plus 32b"
    "a*_64|a*|gen_repeat('a',64)|Single char star 64b"
    "a+_64|a+|gen_repeat('a',64)|Single char plus 64b"
    "a*_128|a*|gen_repeat('a',128)|Single char star 128b"
    "a+_128|a+|gen_repeat('a',128)|Single char plus 128b"
    "a*_256|a*|gen_repeat('a',256)|Single char star 256b"
    "a+_256|a+|gen_repeat('a',256)|Single char plus 256b"
    
    # Sparse character sets (Shufti)
    "[aeiou]*_32|[aeiou]*|gen_sparse(\"aeiou\",5,32)|Vowels star 32b"
    "[aeiou]+_32|[aeiou]+|gen_sparse(\"aeiou\",5,32)|Vowels plus 32b"
    "[aeiou]*_64|[aeiou]*|gen_sparse(\"aeiou\",5,64)|Vowels star 64b"
    "[aeiouAEIOU]*_32|[aeiouAEIOU]*|gen_sparse(\"aeiouAEIOU\",10,32)|Vowels mixed star 32b"
    
    # Alternations (BitNFA)
    "alternation_4|foo|bar|baz|qux|gen_literal(\"bar\")|4-way alternation"
    "complex_alt|GET|POST|PUT|DELETE|PATCH|gen_literal(\"POST\")|HTTP methods"
    "group_alt|(a|b|c)|gen_literal(\"b\")|Grouped alternation"
)

# Generate benchmark template
gen_bench_cpp() {
    local name="$1"
    local pattern="$2"
    local input_expr="$3"
    local desc="$4"
    
    cat << 'BENCH_EOF'
#include <chrono>
#include <iostream>
#include <string>
#include <ctre.hpp>

inline std::string gen_repeat(char c, size_t len) { return std::string(len, c); }
inline std::string gen_range(char start, size_t count, size_t len) {
    std::string result; result.reserve(len);
    for (size_t i = 0; i < len; ++i) result += static_cast<char>(start + (i % count));
    return result;
}
inline std::string gen_sparse(const char* chars, size_t char_count, size_t len) {
    std::string result; result.reserve(len);
    for (size_t i = 0; i < len; ++i) result += chars[i % char_count];
    return result;
}
inline std::string gen_literal(const char* s) { return std::string(s); }

int main() {
    const int ITERATIONS = 100000;
BENCH_EOF
    echo "    std::string test_input = $input_expr;"
    cat << 'BENCH_EOF'
    for (int i = 0; i < 10000; ++i) {
BENCH_EOF
    echo "        volatile bool r = static_cast<bool>(ctre::match<\"$pattern\">(test_input));"
    cat << 'BENCH_EOF'
    }
    double min_time = 1e9;
    for (int sample = 0; sample < 10; ++sample) {
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < ITERATIONS; ++i) {
BENCH_EOF
    echo "            volatile bool r = static_cast<bool>(ctre::match<\"$pattern\">(test_input));"
    cat << 'BENCH_EOF'
        }
        auto end = std::chrono::high_resolution_clock::now();
        double time_ns = std::chrono::duration<double, std::nano>(end - start).count() / ITERATIONS;
        if (time_ns < min_time) min_time = time_ns;
    }
BENCH_EOF
    echo "    std::cout << \"$name\" << \"@@\" << \"$pattern\" << \"@@\" << min_time << \"@@\" << \"$desc\" << std::endl;"
    echo "    return 0;"
    echo "}"
}

total=${#BENCHMARKS[@]}
success=0

for bench in "${BENCHMARKS[@]}"; do
    IFS='|' read -r name pattern input_expr desc <<< "$bench"
    
    # Generate benchmark source
    gen_bench_cpp "$name" "$pattern" "$input_expr" "$desc" > /tmp/bench_${name}.cpp
    
    # Compile SIMD version
    if g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 -flto \
        /tmp/bench_${name}.cpp -o /tmp/${name}_simd 2>/dev/null; then
        
        # Compile NoSIMD version
        if g++ -std=c++20 -Iinclude -O3 -DCTRE_DISABLE_SIMD \
            /tmp/bench_${name}.cpp -o /tmp/${name}_nosimd 2>/dev/null; then
            
            simd_result=$(/tmp/${name}_simd)
            nosimd_result=$(/tmp/${name}_nosimd)
            
            simd_time=$(echo "$simd_result" | cut -d'@' -f5)
            nosimd_time=$(echo "$nosimd_result" | cut -d'@' -f5)
            
            if [[ "$simd_time" =~ ^[0-9.]+$ ]] && [[ "$nosimd_time" =~ ^[0-9.]+$ ]]; then
                speedup=$(echo "scale=2; $nosimd_time / $simd_time" | bc 2>/dev/null)
                
                if [[ -n "$speedup" ]]; then
                    printf "%-40s SIMD: %8.2f ns  NoSIMD: %8.2f ns  Speedup: %5.2fx\n" "$name" "$simd_time" "$nosimd_time" "$speedup"
                    echo "${name}|${simd_time}|${nosimd_time}|${speedup}" >> results/individual/all_results.txt
                    success=$((success + 1))
                fi
            fi
        fi
    fi
    
    rm -f /tmp/bench_${name}.cpp /tmp/${name}_simd /tmp/${name}_nosimd
done

echo ""
echo "============================================"
echo "Successfully benchmarked: $success / $total patterns"
echo "Results saved to results/individual/all_results.txt"
echo "============================================"

if [ -f results/individual/all_results.txt ]; then
    echo ""
    echo "Top 10 SIMD Speedups:"
    sort -t'|' -k4 -rn results/individual/all_results.txt | head -10 | while IFS='|' read -r name simd nosimd speedup; do
        printf "  %-40s %5.2fx\n" "$name" "$speedup"
    done
fi
