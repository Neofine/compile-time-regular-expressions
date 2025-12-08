#!/bin/bash
# Measure binary size impact of SIMD

source "$(dirname "$0")/common.sh"

CXX="${CXX:-g++}"
CFLAGS="-std=c++20 -O3 -march=native -I$PROJECT_ROOT/include -ffunction-sections -fdata-sections"
OUT_DIR="$BUILD_DIR/codesize"
OUT_FILE="$OUTPUT_DIR/codesize.csv"

mkdir -p "$OUT_DIR" "$OUTPUT_DIR"
echo "Pattern,Variant,Total_Bytes,Text_Bytes" > "$OUT_FILE"

measure() {
    local name="$1" pattern="$2"
    cat > "$OUT_DIR/bench_$name.cpp" << EOF
#include <ctre.hpp>
#include <string>
volatile size_t r = 0;
int main(int c, char** v) {
    std::string s(c > 1 ? v[1] : "test");
    for (auto m : ctre::range<"$pattern">(s)) r += m.size();
    return r;
}
EOF
    
    $CXX $CFLAGS -DCTRE_DISABLE_SIMD "$OUT_DIR/bench_$name.cpp" -o "$OUT_DIR/${name}_base" 2>/dev/null
    $CXX $CFLAGS "$OUT_DIR/bench_$name.cpp" -o "$OUT_DIR/${name}_simd" 2>/dev/null
    
    local base_sz=$(stat -c%s "$OUT_DIR/${name}_base" 2>/dev/null || stat -f%z "$OUT_DIR/${name}_base")
    local simd_sz=$(stat -c%s "$OUT_DIR/${name}_simd" 2>/dev/null || stat -f%z "$OUT_DIR/${name}_simd")
    local base_txt=$(size "$OUT_DIR/${name}_base" 2>/dev/null | tail -1 | awk '{print $1}')
    local simd_txt=$(size "$OUT_DIR/${name}_simd" 2>/dev/null | tail -1 | awk '{print $1}')
    
    echo "$name,Baseline,$base_sz,$base_txt" >> "$OUT_FILE"
    echo "$name,SIMD,$simd_sz,$simd_txt" >> "$OUT_FILE"
    echo "  $name: +$((simd_txt - base_txt)) bytes"
}

log "Measuring code size"
measure digits '[0-9]+'
measure lowercase '[a-z]+'
measure alphanumeric '[a-zA-Z0-9]+'
measure hex '[0-9a-fA-F]+'
measure identifier '[a-zA-Z_][a-zA-Z0-9_]*'
measure alt_4 '(a|b|c|d)+'
measure url 'http://[a-z]+'
measure lazy_star '[a-z]*?x'
measure lookahead '[a-z](?=[0-9])'

log "Results: $OUT_FILE"
