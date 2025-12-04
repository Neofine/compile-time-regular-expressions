#!/bin/bash
# Measure code section sizes (not total binary) for different patterns
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLOTS_DIR="$(dirname "$SCRIPT_DIR")"
PROJECT_ROOT="$(dirname "$PLOTS_DIR")"
BUILD_DIR="$PLOTS_DIR/build/codesize"
OUTPUT_FILE="$PLOTS_DIR/output/codesize.csv"

CXX="${CXX:-g++}"
# Use -ffunction-sections to measure code size more accurately
CXXFLAGS="-std=c++20 -O3 -march=native -I$PROJECT_ROOT/include -ffunction-sections -fdata-sections"

mkdir -p "$BUILD_DIR"

echo "Pattern,Variant,Total_Bytes,Text_Bytes" > "$OUTPUT_FILE"

measure_pattern() {
    local name="$1"
    local pattern="$2"

    # Create benchmark file that actually uses the pattern
    cat > "$BUILD_DIR/bench_$name.cpp" << EOF
#include <ctre.hpp>
#include <string>
#include <iostream>

// Force the template to be instantiated
volatile size_t result = 0;

int main(int argc, char** argv) {
    std::string input(argc > 1 ? argv[1] : "test input string 12345");

    for (auto match : ctre::range<"$pattern">(input)) {
        result += match.size();
    }

    std::cout << result << std::endl;
    return 0;
}
EOF

    # Compile baseline (no SIMD) - use CTRE_DISABLE_SIMD
    $CXX $CXXFLAGS -DCTRE_DISABLE_SIMD "$BUILD_DIR/bench_$name.cpp" -o "$BUILD_DIR/${name}_base" 2>/dev/null
    local base_total=$(stat -c%s "$BUILD_DIR/${name}_base" 2>/dev/null || stat -f%z "$BUILD_DIR/${name}_base")
    local base_text=$(size "$BUILD_DIR/${name}_base" 2>/dev/null | tail -1 | awk '{print $1}')

    # Compile with SIMD (default, no flag needed)
    $CXX $CXXFLAGS "$BUILD_DIR/bench_$name.cpp" -o "$BUILD_DIR/${name}_simd" 2>/dev/null
    local simd_total=$(stat -c%s "$BUILD_DIR/${name}_simd" 2>/dev/null || stat -f%z "$BUILD_DIR/${name}_simd")
    local simd_text=$(size "$BUILD_DIR/${name}_simd" 2>/dev/null | tail -1 | awk '{print $1}')

    echo "$name,Baseline,$base_total,$base_text" >> "$OUTPUT_FILE"
    echo "$name,SIMD,$simd_total,$simd_text" >> "$OUTPUT_FILE"

    local diff=$((simd_text - base_text))
    echo "  $name: base=${base_text}B, simd=${simd_text}B (diff: ${diff}B)"
}

echo "Measuring code sizes..."
echo ""

# Simple patterns
measure_pattern "digits" '[0-9]+'
measure_pattern "lowercase" '[a-z]+'
measure_pattern "alphanumeric" '[a-zA-Z0-9]+'

# Complex patterns
measure_pattern "decimal" '[0-9]+\.[0-9]+'
measure_pattern "hex" '[0-9a-fA-F]+'
measure_pattern "identifier" '[a-zA-Z_][a-zA-Z0-9_]*'

# Multi-alternation
measure_pattern "alt_4" '(a|b|c|d)+'
measure_pattern "alt_8" '(a|b|c|d|e|f|g|h)+'

# URL pattern (more complex)
measure_pattern "url" 'http://[a-z]+'

echo ""
echo "=== Fallback patterns (SIMD-ineligible, should show no code increase) ==="

# Non-greedy quantifier - requires backtracking
measure_pattern "lazy_star" '[a-z]*?x'

# Lookahead - position-dependent
measure_pattern "lookahead" '[a-z](?=[0-9])'

# Capturing group repetition
measure_pattern "group_repeat" '(abc)+'

echo ""
echo "=== Checking if SIMD code differs ==="
echo "Comparing .text sections..."
for name in digits lowercase alphanumeric; do
    base_md5=$(objdump -d "$BUILD_DIR/${name}_base" 2>/dev/null | md5sum | cut -d' ' -f1)
    simd_md5=$(objdump -d "$BUILD_DIR/${name}_simd" 2>/dev/null | md5sum | cut -d' ' -f1)
    if [ "$base_md5" = "$simd_md5" ]; then
        echo "  $name: SAME (SIMD not being used!)"
    else
        echo "  $name: DIFFERENT (SIMD active)"
    fi
done

echo ""
echo "Results saved to: $OUTPUT_FILE"
cat "$OUTPUT_FILE"
