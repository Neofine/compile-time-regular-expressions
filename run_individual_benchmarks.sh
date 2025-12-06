#!/bin/bash
cd "$(dirname "$0")"

echo "Running individual SIMD benchmarks..."
mkdir -p results/individual
rm -f results/individual/all_results.txt

BENCHMARKS=(
    "[0-9]*_32|[0-9]*|gen_range('0',10,32)"
    "[0-9]+_32|[0-9]+|gen_range('0',10,32)"
    "[0-9]*_256|[0-9]*|gen_range('0',10,256)"
    "[0-9]+_256|[0-9]+|gen_range('0',10,256)"
    "[a-z]*_32|[a-z]*|gen_range('a',26,32)"
    "[a-z]+_32|[a-z]+|gen_range('a',26,32)"
    "[a-z]*_128|[a-z]*|gen_range('a',26,128)"
    "[a-z]+_128|[a-z]+|gen_range('a',26,128)"
    "[a-z]*_256|[a-z]*|gen_range('a',26,256)"
    "[a-z]+_256|[a-z]+|gen_range('a',26,256)"
    "[a-z]*_512|[a-z]*|gen_range('a',26,512)"
    "[a-z]+_512|[a-z]+|gen_range('a',26,512)"
    "[A-Z]*_32|[A-Z]*|gen_range('A',26,32)"
    "[A-Z]+_32|[A-Z]+|gen_range('A',26,32)"
    "[A-Z]*_256|[A-Z]*|gen_range('A',26,256)"
    "[a-zA-Z]*_32|[a-zA-Z]*|gen_range('a',26,32)"
    "[a-zA-Z]+_32|[a-zA-Z]+|gen_range('a',26,32)"
    "[a-zA-Z]*_64|[a-zA-Z]*|gen_range('a',26,64)"
    "[a-zA-Z]+_64|[a-zA-Z]+|gen_range('a',26,64)"
    "[a-zA-Z]*_128|[a-zA-Z]*|gen_range('a',26,128)"
    "[0-9a-f]*_32|[0-9a-f]*|gen_range('0',16,32)"
    "[0-9a-f]+_32|[0-9a-f]+|gen_range('0',16,32)"
    "[0-9a-f]*_64|[0-9a-f]*|gen_range('0',16,64)"
    "[0-9a-fA-F]*_32|[0-9a-fA-F]*|gen_range('0',16,32)"
    "[0-9a-fA-F]+_32|[0-9a-fA-F]+|gen_range('0',16,32)"
    "a*_32|a*|gen_repeat('a',32)"
    "a+_32|a+|gen_repeat('a',32)"
    "a*_64|a*|gen_repeat('a',64)"
    "a+_64|a+|gen_repeat('a',64)"
    "a*_128|a*|gen_repeat('a',128)"
    "a+_128|a+|gen_repeat('a',128)"
    "a*_256|a*|gen_repeat('a',256)"
    "a+_256|a+|gen_repeat('a',256)"
    "[aeiou]*_32|[aeiou]*|gen_sparse(\"aeiou\",5,32)"
    "[aeiou]+_32|[aeiou]+|gen_sparse(\"aeiou\",5,32)"
    "[aeiou]*_64|[aeiou]*|gen_sparse(\"aeiou\",5,64)"
    "[aeiouAEIOU]*_32|[aeiouAEIOU]*|gen_sparse(\"aeiouAEIOU\",10,32)"
    "alternation_4|foo|bar|baz|qux|gen_literal(\"bar\")"
    "complex_alt|GET|POST|PUT|DELETE|PATCH|gen_literal(\"POST\")"
    "group_alt|(a|b|c)|gen_literal(\"b\")"
)

gen_bench_cpp() {
    local name="$1" pattern="$2" input_expr="$3"
    cat << 'EOF'
#include <chrono>
#include <iostream>
#include <string>
#include <ctre.hpp>

inline std::string gen_repeat(char c, size_t len) { return std::string(len, c); }
inline std::string gen_range(char start, size_t count, size_t len) {
    std::string r; r.reserve(len);
    for (size_t i = 0; i < len; ++i) r += static_cast<char>(start + (i % count));
    return r;
}
inline std::string gen_sparse(const char* chars, size_t n, size_t len) {
    std::string r; r.reserve(len);
    for (size_t i = 0; i < len; ++i) r += chars[i % n];
    return r;
}
inline std::string gen_literal(const char* s) { return std::string(s); }

int main() {
    const int ITER = 100000;
EOF
    echo "    std::string input = $input_expr;"
    echo "    for (int i = 0; i < 10000; ++i) volatile bool r = static_cast<bool>(ctre::match<\"$pattern\">(input));"
    echo "    double min_t = 1e9;"
    echo "    for (int s = 0; s < 10; ++s) {"
    echo "        auto t0 = std::chrono::high_resolution_clock::now();"
    echo "        for (int i = 0; i < ITER; ++i) volatile bool r = static_cast<bool>(ctre::match<\"$pattern\">(input));"
    echo "        auto t1 = std::chrono::high_resolution_clock::now();"
    echo "        double ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / ITER;"
    echo "        if (ns < min_t) min_t = ns;"
    echo "    }"
    echo "    std::cout << \"$name\" << \"@@\" << min_t << std::endl;"
    echo "    return 0;"
    echo "}"
}

total=${#BENCHMARKS[@]}
success=0

for bench in "${BENCHMARKS[@]}"; do
    IFS='|' read -r name pattern input_expr <<< "$bench"
    gen_bench_cpp "$name" "$pattern" "$input_expr" > /tmp/bench_${name}.cpp

    if g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 -flto /tmp/bench_${name}.cpp -o /tmp/${name}_simd 2>/dev/null; then
        if g++ -std=c++20 -Iinclude -O3 -DCTRE_DISABLE_SIMD /tmp/bench_${name}.cpp -o /tmp/${name}_nosimd 2>/dev/null; then
            simd_time=$(echo "$(/tmp/${name}_simd)" | cut -d'@' -f3)
            nosimd_time=$(echo "$(/tmp/${name}_nosimd)" | cut -d'@' -f3)

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
echo "Benchmarked: $success / $total"

if [ -f results/individual/all_results.txt ]; then
    echo "Top 10 speedups:"
    sort -t'|' -k4 -rn results/individual/all_results.txt | head -10 | while IFS='|' read -r name simd nosimd speedup; do
        printf "  %-40s %5.2fx\n" "$name" "$speedup"
    done
fi
