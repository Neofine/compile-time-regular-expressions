#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLOTS_DIR="$(dirname "$SCRIPT_DIR")"
PROJECT_ROOT="$(dirname "$PLOTS_DIR")"
BUILD_DIR="$PLOTS_DIR/build/compile_time"
OUTPUT_FILE="$PLOTS_DIR/output/compile_time.csv"

CXX="${CXX:-g++}"
CXXFLAGS="-std=c++20 -O3 -march=native -I$PROJECT_ROOT/include"
RUNS=3

mkdir -p "$BUILD_DIR" "$(dirname "$OUTPUT_FILE")"
echo "Pattern,Category,Variant,Compile_Time_s,Binary_Size_KB" > "$OUTPUT_FILE"

measure() {
    local name="$1" pattern="$2" category="$3"
    cat > "$BUILD_DIR/bench_$name.cpp" << EOF
#include <ctre.hpp>
#include <string>
volatile size_t result = 0;
int main(int argc, char** argv) {
    std::string input(argc > 1 ? argv[1] : "test");
    if (ctre::match<"$pattern">(input)) result = 1;
    return result;
}
EOF

    local base_t=0 simd_t=0
    for ((i=1; i<=RUNS; i++)); do
        rm -f "$BUILD_DIR/${name}_base" 2>/dev/null
        local t0=$(date +%s.%N)
        $CXX $CXXFLAGS -DCTRE_DISABLE_SIMD "$BUILD_DIR/bench_$name.cpp" -o "$BUILD_DIR/${name}_base" 2>/dev/null
        base_t=$(echo "$base_t + $(date +%s.%N) - $t0" | bc)
    done
    base_t=$(echo "scale=3; $base_t / $RUNS" | bc)
    local base_kb=$(echo "scale=1; $(stat -c%s "$BUILD_DIR/${name}_base" 2>/dev/null || stat -f%z "$BUILD_DIR/${name}_base") / 1024" | bc)

    for ((i=1; i<=RUNS; i++)); do
        rm -f "$BUILD_DIR/${name}_simd" 2>/dev/null
        local t0=$(date +%s.%N)
        $CXX $CXXFLAGS "$BUILD_DIR/bench_$name.cpp" -o "$BUILD_DIR/${name}_simd" 2>/dev/null
        simd_t=$(echo "$simd_t + $(date +%s.%N) - $t0" | bc)
    done
    simd_t=$(echo "scale=3; $simd_t / $RUNS" | bc)
    local simd_kb=$(echo "scale=1; $(stat -c%s "$BUILD_DIR/${name}_simd" 2>/dev/null || stat -f%z "$BUILD_DIR/${name}_simd") / 1024" | bc)

    echo "$name,$category,Baseline,$base_t,$base_kb" >> "$OUTPUT_FILE"
    echo "$name,$category,SIMD,$simd_t,$simd_kb" >> "$OUTPUT_FILE"
    local oh=$(echo "scale=1; ($simd_t - $base_t) / $base_t * 100" | bc 2>/dev/null || echo "N/A")
    echo "  $name: base=${base_t}s simd=${simd_t}s (${oh}%)"
}

echo "Measuring compile time ($RUNS runs each)..."

echo "Simple:"
measure "digits" '[0-9]+' "Simple"
measure "lowercase" '[a-z]+' "Simple"
measure "uppercase" '[A-Z]+' "Simple"
measure "vowels" '[aeiou]+' "Simple"
measure "alphanumeric" '[a-zA-Z0-9]+' "Simple"

echo "Complex:"
measure "decimal" '[0-9]+\.[0-9]+' "Complex"
measure "hex" '[0-9a-fA-F]+' "Complex"
measure "identifier" '[a-zA-Z_][a-zA-Z0-9_]*' "Complex"
measure "url" 'http://[a-z]+' "Complex"
measure "key_value" '[a-z]+=[0-9]+' "Complex"
measure "ipv4" '[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+' "Complex"

echo "Behemoth:"
measure "alt_8" '(a|b|c|d|e|f|g|h)+' "Behemoth"
measure "uuid" '[0-9a-f]+-[0-9a-f]+-[0-9a-f]+-[0-9a-f]+-[0-9a-f]+' "Behemoth"
measure "email" '[a-zA-Z0-9._]+@[a-zA-Z0-9.]+[.][a-zA-Z]+' "Behemoth"
measure "http_log" '[A-Z]+ /[a-zA-Z0-9/]+ HTTP/[0-9.]+' "Behemoth"
measure "config" '[a-zA-Z_][a-zA-Z0-9_]*=[a-zA-Z0-9_]+' "Behemoth"
measure "datetime" '[0-9]+-[0-9]+-[0-9]+T[0-9]+:[0-9]+:[0-9]+' "Behemoth"

echo "Results: $OUTPUT_FILE"
