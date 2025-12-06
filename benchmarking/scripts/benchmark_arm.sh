#!/bin/bash
# ARM benchmark - outputs CSV compatible with generate.py
# Tests CTRE-Scalar (optimized loops) vs CTRE (original recursive templates)
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
OUTPUT_DIR="$SCRIPT_DIR/../output/arm"

echo "ARM Benchmark - CTRE-Scalar vs CTRE (original)"
echo "Platform: $(uname -m)"

# Use clang++ on Mac, g++ otherwise
if [[ "$(uname)" == "Darwin" ]]; then
    CXX="clang++"
else
    CXX="g++"
fi
echo "Compiler: $CXX"

mkdir -p "$OUTPUT_DIR"
SCALAR_CSV="$OUTPUT_DIR/scalar.csv"
ORIG_CSV="$OUTPUT_DIR/original.csv"

echo "Pattern,Engine,Input_Size,Time_ns,Matches" > "$SCALAR_CSV"
echo "Pattern,Engine,Input_Size,Time_ns,Matches" > "$ORIG_CSV"

# Matching patterns - character class repetitions
# Format: name@pattern@input_chars
MATCH_PATTERNS=(
    "digits@[0-9]+@0123456789"
    "lowercase@[a-z]+@abcdefghijklmnopqrstuvwxyz"
    "uppercase@[A-Z]+@ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "alphanumeric@[a-zA-Z0-9]+@abcABC123"
    "vowels@[aeiou]+@aeiou"
    "hex@[0-9a-fA-F]+@0123456789abcdef"
)

# Non-matching patterns - test early rejection and prefiltering
NOMATCH_PATTERNS=(
    # First-char rejection
    "nomatch_digits@[0-9]+@abcdefghijklmnopqrstuvwxyz"
    "nomatch_alpha@[a-zA-Z]+@0123456789"
    # Region analysis prefilter - input lacks 'est'
    "region_reject@[a-z]*(test|fest|best)@abcdfghijklmnopqruvwxyz"
    # Dominator analysis prefilter - input lacks 'test'  
    "dom_reject@[a-z]*test[a-z]*@abcdfghijklmnopqruvwxyz"
)

# Sizes from 2^2 to 2^15
SIZES=(4 8 16 32 64 128 256 512 1024 2048 4096 8192 16384 32768)

# Test compile first
echo "Testing compilation..."
cat > /tmp/arm_test.cpp << 'EOF'
#include <ctre.hpp>
#include <string>
int main() {
    std::string s = "123";
    return ctre::match<"[0-9]+">(s) ? 0 : 1;
}
EOF

if ! $CXX -std=c++20 -O2 -I "$PROJECT_ROOT/include" /tmp/arm_test.cpp -o /tmp/arm_test 2>&1; then
    echo "ERROR: Compilation failed. Check your compiler supports C++20."
    exit 1
fi
echo "Compilation OK"
echo ""

run_benchmark() {
    local prefix=$1
    local name=$2
    local pattern=$3
    local chars=$4
    
    for size in "${SIZES[@]}"; do
        cat > /tmp/arm_bench.cpp << EOF
#include <chrono>
#include <iostream>
#include <string>
#include <cstring>
#include <ctre.hpp>

// Single input, repeated - realistic usage pattern
// This avoids artificial branch misprediction from diverse inputs
int main() {
    std::string input;
    const char* chars = "$chars";
    size_t len = strlen(chars);
    for (size_t i = 0; i < $size; ++i) input += chars[i % len];
    
    auto result = ctre::match<"$pattern">(input);
    int matched = result ? 1 : 0;
    
    // Warmup
    for (int i = 0; i < 1000; ++i) volatile bool r = ctre::match<"$pattern">(input);
    
    // Benchmark: single input repeated (realistic branch prediction)
    const int ITERS = 10000;
    auto t0 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < ITERS; ++i) {
        volatile auto r = ctre::match<"$pattern">(input);
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    double ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / ITERS;
    
    std::cout << ns << "," << matched << std::endl;
    return 0;
}
EOF
        
        # Scalar version (optimized loops, no SIMD instructions on ARM)
        if $CXX -std=c++20 -O3 -I "$PROJECT_ROOT/include" /tmp/arm_bench.cpp -o /tmp/arm_scalar 2>/tmp/arm_err; then
            if result=$(/tmp/arm_scalar 2>&1); then
                time_ns=$(echo "$result" | cut -d',' -f1)
                match=$(echo "$result" | cut -d',' -f2)
                echo "${prefix}/${name},CTRE-Scalar,$size,$time_ns,$match" >> "$SCALAR_CSV"
            fi
        else
            echo ""
            echo "Compile error for $name size=$size (scalar):"
            cat /tmp/arm_err | head -20
        fi
        
        # Original CTRE (recursive templates)
        if $CXX -std=c++20 -O3 -DCTRE_DISABLE_SIMD -I "$PROJECT_ROOT/include" /tmp/arm_bench.cpp -o /tmp/arm_orig 2>/tmp/arm_err; then
            if result=$(/tmp/arm_orig 2>&1); then
                time_ns=$(echo "$result" | cut -d',' -f1)
                match=$(echo "$result" | cut -d',' -f2)
                echo "${prefix}/${name},CTRE,$size,$time_ns,$match" >> "$ORIG_CSV"
            fi
        else
            echo ""
            echo "Compile error for $name size=$size (original):"
            head -3 /tmp/arm_err
        fi
        
        echo -n "."
    done
    echo " $name done"
}

# Run matching patterns
echo "=== Matching Patterns ==="
for p in "${MATCH_PATTERNS[@]}"; do
    IFS='@' read -r name pattern chars <<< "$p"
    run_benchmark "arm" "$name" "$pattern" "$chars"
done

# Run non-matching patterns  
echo ""
echo "=== Non-Matching Patterns ==="
for p in "${NOMATCH_PATTERNS[@]}"; do
    IFS='@' read -r name pattern chars <<< "$p"
    run_benchmark "arm_nomatch" "$name" "$pattern" "$chars"
done

echo ""
echo "Done! CSV files:"
wc -l "$SCALAR_CSV" "$ORIG_CSV"
echo ""
head -5 "$SCALAR_CSV"
echo "..."
echo ""
echo "Now run: cd $SCRIPT_DIR/.. && python3 generate.py"
