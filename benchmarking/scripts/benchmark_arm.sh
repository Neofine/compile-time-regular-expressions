#!/bin/bash
# ARM benchmark - outputs CSV compatible with generate.py
set -e

# Get script directory and project root
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
OUTPUT_DIR="$SCRIPT_DIR/../output/arm"

echo "ARM Benchmark - CTRE SIMD vs Baseline"
echo "Platform: $(uname -m)"
echo "Script dir: $SCRIPT_DIR"
echo "Project root: $PROJECT_ROOT"
echo "Output dir: $OUTPUT_DIR"

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

SIZES=(32 64 128 256 512)

for p in "${PATTERNS[@]}"; do
    IFS='|' read -r name pattern chars <<< "$p"
    
    for size in "${SIZES[@]}"; do
        cat > /tmp/arm_bench.cpp << EOF
#include <chrono>
#include <iostream>
#include <string>
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
        
        # SIMD version
        if g++ -std=c++20 -O3 -I "$PROJECT_ROOT/include" /tmp/arm_bench.cpp -o /tmp/arm_simd 2>/dev/null; then
            result=$(/tmp/arm_simd)
            time_ns=$(echo "$result" | cut -d',' -f1)
            match=$(echo "$result" | cut -d',' -f2)
            echo "arm/$name,CTRE,$size,$time_ns,$match" >> "$SIMD_CSV"
        fi
        
        # Baseline version
        if g++ -std=c++20 -O3 -DCTRE_DISABLE_SIMD -I "$PROJECT_ROOT/include" /tmp/arm_bench.cpp -o /tmp/arm_base 2>/dev/null; then
            result=$(/tmp/arm_base)
            time_ns=$(echo "$result" | cut -d',' -f1)
            match=$(echo "$result" | cut -d',' -f2)
            echo "arm/$name,CTRE,$size,$time_ns,$match" >> "$BASE_CSV"
        fi
        
        echo -n "."
    done
    echo " $name done"
done

echo ""
echo "Done! CSV files:"
echo "  $SIMD_CSV"
echo "  $BASE_CSV"
echo ""
echo "Now run: cd $SCRIPT_DIR/.. && python3 generate.py"

