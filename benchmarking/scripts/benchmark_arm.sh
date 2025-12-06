#!/bin/bash
# ARM benchmark - outputs CSV compatible with generate.py
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
OUTPUT_DIR="$SCRIPT_DIR/../output/arm"

echo "ARM Benchmark - CTRE SIMD vs Baseline"
echo "Platform: $(uname -m)"

# Use clang++ on Mac, g++ otherwise
if [[ "$(uname)" == "Darwin" ]]; then
    CXX="clang++"
else
    CXX="g++"
fi
echo "Compiler: $CXX"

mkdir -p "$OUTPUT_DIR"
SIMD_CSV="$OUTPUT_DIR/simd.csv"
BASE_CSV="$OUTPUT_DIR/baseline.csv"

echo "Pattern,Engine,Input_Size,Time_ns,Matches" > "$SIMD_CSV"
echo "Pattern,Engine,Input_Size,Time_ns,Matches" > "$BASE_CSV"

PATTERNS=(
    "digits|[0-9]+|0123456789"
    "lowercase|[a-z]+|abcdefghijklmnopqrstuvwxyz"
    "uppercase|[A-Z]+|ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "alphanumeric|[a-zA-Z0-9]+|abcABC123"
    "vowels|[aeiou]+|aeiou"
    "hex|[0-9a-fA-F]+|0123456789abcdef"
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

for p in "${PATTERNS[@]}"; do
    IFS='|' read -r name pattern chars <<< "$p"
    
    for size in "${SIZES[@]}"; do
        cat > /tmp/arm_bench.cpp << EOF
#include <chrono>
#include <iostream>
#include <string>
#include <cstring>
#include <ctre.hpp>

int main() {
    std::string input;
    const char* chars = "$chars";
    size_t len = strlen(chars);
    for (size_t i = 0; i < $size; ++i) input += chars[i % len];
    
    for (int i = 0; i < 1000; ++i) volatile bool r = ctre::match<"$pattern">(input);
    
    double best = 1e9;
    int matches = 0;
    for (int s = 0; s < 5; ++s) {
        auto t0 = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < 10000; ++i) {
            if (ctre::match<"$pattern">(input)) matches++;
        }
        auto t1 = std::chrono::high_resolution_clock::now();
        double ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / 10000;
        if (ns < best) best = ns;
    }
    std::cout << best << "," << (matches > 0 ? 1 : 0) << std::endl;
    return 0;
}
EOF
        
        # SIMD version (default build - on ARM falls back to scalar)
        if $CXX -std=c++20 -O3 -I "$PROJECT_ROOT/include" /tmp/arm_bench.cpp -o /tmp/arm_simd 2>/tmp/arm_err; then
            if result=$(/tmp/arm_simd 2>&1); then
                time_ns=$(echo "$result" | cut -d',' -f1)
                match=$(echo "$result" | cut -d',' -f2)
                echo "arm/$name,CTRE-SIMD,$size,$time_ns,$match" >> "$SIMD_CSV"
            fi
        else
            echo ""
            echo "Compile error for $name size=$size (SIMD):"
            cat /tmp/arm_err | head -20
        fi
        
        # Baseline version (explicitly disabled SIMD)
        if $CXX -std=c++20 -O3 -DCTRE_DISABLE_SIMD -I "$PROJECT_ROOT/include" /tmp/arm_bench.cpp -o /tmp/arm_base 2>/tmp/arm_err; then
            if result=$(/tmp/arm_base 2>&1); then
                time_ns=$(echo "$result" | cut -d',' -f1)
                match=$(echo "$result" | cut -d',' -f2)
                echo "arm/$name,CTRE,$size,$time_ns,$match" >> "$BASE_CSV"
            fi
        else
            echo ""
            echo "Compile error for $name size=$size (baseline):"
            head -3 /tmp/arm_err
        fi
        
        echo -n "."
    done
    echo " $name done"
done

echo ""
echo "Done! CSV files:"
wc -l "$SIMD_CSV" "$BASE_CSV"
echo ""
head -5 "$SIMD_CSV"
echo "..."
echo ""
echo "Now run: cd $SCRIPT_DIR/.. && python3 generate.py"
