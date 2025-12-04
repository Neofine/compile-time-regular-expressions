#!/bin/bash
# Measure compilation time for different patterns (SIMD vs baseline)
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLOTS_DIR="$(dirname "$SCRIPT_DIR")"
PROJECT_ROOT="$(dirname "$PLOTS_DIR")"
BUILD_DIR="$PLOTS_DIR/build/compile_time"
OUTPUT_FILE="$PLOTS_DIR/output/compile_time.csv"

CXX="${CXX:-g++}"
CXXFLAGS="-std=c++20 -O3 -march=native -I$PROJECT_ROOT/include"

# Number of compilation runs for averaging
RUNS=3

mkdir -p "$BUILD_DIR"
mkdir -p "$(dirname "$OUTPUT_FILE")"

echo "Pattern,Category,Variant,Compile_Time_s,Binary_Size_KB" > "$OUTPUT_FILE"

measure_pattern() {
    local name="$1"
    local pattern="$2"
    local category="$3"

    # Create benchmark file
    cat > "$BUILD_DIR/bench_$name.cpp" << EOF
#include <ctre.hpp>
#include <string>
#include <iostream>

volatile size_t result = 0;

int main(int argc, char** argv) {
    std::string input(argc > 1 ? argv[1] : "test input string 12345");

    if (ctre::match<"$pattern">(input)) {
        result = 1;
    }

    return result;
}
EOF

    # Measure baseline compilation time (average of RUNS)
    local base_total_time=0
    for ((i=1; i<=RUNS; i++)); do
        rm -f "$BUILD_DIR/${name}_base" 2>/dev/null || true
        local start=$(date +%s.%N)
        $CXX $CXXFLAGS -DCTRE_DISABLE_SIMD "$BUILD_DIR/bench_$name.cpp" -o "$BUILD_DIR/${name}_base" 2>/dev/null
        local end=$(date +%s.%N)
        base_total_time=$(echo "$base_total_time + $end - $start" | bc)
    done
    local base_time=$(echo "scale=3; $base_total_time / $RUNS" | bc)
    local base_size=$(stat -c%s "$BUILD_DIR/${name}_base" 2>/dev/null || stat -f%z "$BUILD_DIR/${name}_base")
    local base_size_kb=$(echo "scale=1; $base_size / 1024" | bc)

    # Measure SIMD compilation time (average of RUNS)
    local simd_total_time=0
    for ((i=1; i<=RUNS; i++)); do
        rm -f "$BUILD_DIR/${name}_simd" 2>/dev/null || true
        local start=$(date +%s.%N)
        $CXX $CXXFLAGS "$BUILD_DIR/bench_$name.cpp" -o "$BUILD_DIR/${name}_simd" 2>/dev/null
        local end=$(date +%s.%N)
        simd_total_time=$(echo "$simd_total_time + $end - $start" | bc)
    done
    local simd_time=$(echo "scale=3; $simd_total_time / $RUNS" | bc)
    local simd_size=$(stat -c%s "$BUILD_DIR/${name}_simd" 2>/dev/null || stat -f%z "$BUILD_DIR/${name}_simd")
    local simd_size_kb=$(echo "scale=1; $simd_size / 1024" | bc)

    echo "$name,$category,Baseline,$base_time,$base_size_kb" >> "$OUTPUT_FILE"
    echo "$name,$category,SIMD,$simd_time,$simd_size_kb" >> "$OUTPUT_FILE"

    local overhead=$(echo "scale=1; ($simd_time - $base_time) / $base_time * 100" | bc 2>/dev/null || echo "N/A")
    echo "  $name: baseline=${base_time}s, simd=${simd_time}s (overhead: ${overhead}%)"
}

echo "=============================================="
echo "Measuring Compilation Time (average of $RUNS runs)"
echo "=============================================="
echo ""

# ============================================
# SIMPLE PATTERNS
# ============================================
echo "=== Simple Patterns ==="
measure_pattern "digits" '[0-9]+' "Simple"
measure_pattern "lowercase" '[a-z]+' "Simple"
measure_pattern "uppercase" '[A-Z]+' "Simple"
measure_pattern "vowels" '[aeiou]+' "Simple"
measure_pattern "alphanumeric" '[a-zA-Z0-9]+' "Simple"

echo ""

# ============================================
# COMPLEX PATTERNS
# ============================================
echo "=== Complex Patterns ==="
measure_pattern "decimal" '[0-9]+\.[0-9]+' "Complex"
measure_pattern "hex" '[0-9a-fA-F]+' "Complex"
measure_pattern "identifier" '[a-zA-Z_][a-zA-Z0-9_]*' "Complex"
measure_pattern "url" 'http://[a-z]+' "Complex"
measure_pattern "key_value" '[a-z]+=[0-9]+' "Complex"
measure_pattern "ipv4" '[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+' "Complex"

echo ""

# ============================================
# BEHEMOTH PATTERNS (complex, nested, long)
# ============================================
echo "=== Behemoth Patterns ==="

# Multi-alternation (stress test)
measure_pattern "alt_8" '(a|b|c|d|e|f|g|h)+' "Behemoth"

# UUID pattern (5 groups)
measure_pattern "uuid" '[0-9a-f]+-[0-9a-f]+-[0-9a-f]+-[0-9a-f]+-[0-9a-f]+' "Behemoth"

# Email pattern
measure_pattern "email" '[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]+' "Behemoth"

# HTTP log pattern
measure_pattern "http_log" '[A-Z]+\s+/[a-zA-Z0-9/]+\s+HTTP/[0-9.]+' "Behemoth"

# Config line (key=value with optional quotes)
measure_pattern "config" '[a-zA-Z_][a-zA-Z0-9_]*=[a-zA-Z0-9_]+' "Behemoth"

# Date-time pattern
measure_pattern "datetime" '[0-9]+-[0-9]+-[0-9]+T[0-9]+:[0-9]+:[0-9]+' "Behemoth"

echo ""
echo "=============================================="
echo "Results saved to: $OUTPUT_FILE"
echo "=============================================="
echo ""
cat "$OUTPUT_FILE"

