#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLOTS_DIR="$(dirname "$SCRIPT_DIR")"
PROJECT_ROOT="$(dirname "$PLOTS_DIR")"
BUILD_DIR="$PLOTS_DIR/build/codesize"
OUTPUT_FILE="$PLOTS_DIR/output/codesize.csv"

CXX="${CXX:-g++}"
CXXFLAGS="-std=c++20 -O3 -march=native -I$PROJECT_ROOT/include -ffunction-sections -fdata-sections"

mkdir -p "$BUILD_DIR" "$(dirname "$OUTPUT_FILE")"
echo "Pattern,Variant,Total_Bytes,Text_Bytes" > "$OUTPUT_FILE"

measure() {
    local name="$1" pattern="$2"
    cat > "$BUILD_DIR/bench_$name.cpp" << EOF
#include <ctre.hpp>
#include <string>
#include <iostream>
volatile size_t result = 0;
int main(int argc, char** argv) {
    std::string input(argc > 1 ? argv[1] : "test input string 12345");
    for (auto match : ctre::range<"$pattern">(input)) result += match.size();
    std::cout << result << std::endl;
    return 0;
}
EOF

    $CXX $CXXFLAGS -DCTRE_DISABLE_SIMD "$BUILD_DIR/bench_$name.cpp" -o "$BUILD_DIR/${name}_base" 2>/dev/null
    local base_total=$(stat -c%s "$BUILD_DIR/${name}_base" 2>/dev/null || stat -f%z "$BUILD_DIR/${name}_base")
    local base_text=$(size "$BUILD_DIR/${name}_base" 2>/dev/null | tail -1 | awk '{print $1}')

    $CXX $CXXFLAGS "$BUILD_DIR/bench_$name.cpp" -o "$BUILD_DIR/${name}_simd" 2>/dev/null
    local simd_total=$(stat -c%s "$BUILD_DIR/${name}_simd" 2>/dev/null || stat -f%z "$BUILD_DIR/${name}_simd")
    local simd_text=$(size "$BUILD_DIR/${name}_simd" 2>/dev/null | tail -1 | awk '{print $1}')

    echo "$name,Baseline,$base_total,$base_text" >> "$OUTPUT_FILE"
    echo "$name,SIMD,$simd_total,$simd_text" >> "$OUTPUT_FILE"
    echo "  $name: base=${base_text}B simd=${simd_text}B diff=$((simd_text - base_text))B"
}

echo "Measuring code sizes..."
measure "digits" '[0-9]+'
measure "lowercase" '[a-z]+'
measure "alphanumeric" '[a-zA-Z0-9]+'
measure "decimal" '[0-9]+\.[0-9]+'
measure "hex" '[0-9a-fA-F]+'
measure "identifier" '[a-zA-Z_][a-zA-Z0-9_]*'
measure "alt_4" '(a|b|c|d)+'
measure "alt_8" '(a|b|c|d|e|f|g|h)+'
measure "url" 'http://[a-z]+'

echo "Fallback patterns (should show no increase):"
measure "lazy_star" '[a-z]*?x'
measure "lookahead" '[a-z](?=[0-9])'
measure "group_repeat" '(abc)+'

echo "Results: $OUTPUT_FILE"
