#!/bin/bash
# Measure compile time overhead of SIMD

source "$(dirname "$0")/common.sh"

CXX="${CXX:-g++}"
CFLAGS="-std=c++20 -O3 -march=native -I$PROJECT_ROOT/include"
RUNS=3
OUT_DIR="$BUILD_DIR/compile_time"
OUT_FILE="$OUTPUT_DIR/compile_time.csv"

mkdir -p "$OUT_DIR" "$OUTPUT_DIR"
echo "Pattern,Category,Variant,Compile_Time_s,Binary_Size_KB" > "$OUT_FILE"

measure() {
    local name="$1" pattern="$2" category="$3"
    cat > "$OUT_DIR/bench_$name.cpp" << EOF
#include <ctre.hpp>
#include <string>
volatile size_t r = 0;
int main(int c, char** v) {
    if (ctre::match<"$pattern">(std::string(c > 1 ? v[1] : "test"))) r = 1;
    return r;
}
EOF
    
    local base_t=0 simd_t=0
    for ((i=1; i<=RUNS; i++)); do
        rm -f "$OUT_DIR/${name}_base" 2>/dev/null
        local t0=$(date +%s.%N)
        $CXX $CFLAGS -DCTRE_DISABLE_SIMD "$OUT_DIR/bench_$name.cpp" -o "$OUT_DIR/${name}_base" 2>/dev/null
        base_t=$(echo "$base_t + $(date +%s.%N) - $t0" | bc)
        
        rm -f "$OUT_DIR/${name}_simd" 2>/dev/null
        t0=$(date +%s.%N)
        $CXX $CFLAGS "$OUT_DIR/bench_$name.cpp" -o "$OUT_DIR/${name}_simd" 2>/dev/null
        simd_t=$(echo "$simd_t + $(date +%s.%N) - $t0" | bc)
    done
    
    base_t=$(echo "scale=3; $base_t / $RUNS" | bc)
    simd_t=$(echo "scale=3; $simd_t / $RUNS" | bc)
    local base_kb=$(echo "scale=1; $(stat -c%s "$OUT_DIR/${name}_base" 2>/dev/null || stat -f%z "$OUT_DIR/${name}_base") / 1024" | bc)
    local simd_kb=$(echo "scale=1; $(stat -c%s "$OUT_DIR/${name}_simd" 2>/dev/null || stat -f%z "$OUT_DIR/${name}_simd") / 1024" | bc)
    
    echo "$name,$category,Baseline,$base_t,$base_kb" >> "$OUT_FILE"
    echo "$name,$category,SIMD,$simd_t,$simd_kb" >> "$OUT_FILE"
    echo "  $name: ${base_t}s → ${simd_t}s"
}

log "Measuring compile time ($RUNS runs)"

measure digits '[0-9]+' Simple
measure lowercase '[a-z]+' Simple
measure alphanumeric '[a-zA-Z0-9]+' Simple
measure hex '[0-9a-fA-F]+' Complex
measure identifier '[a-zA-Z_][a-zA-Z0-9_]*' Complex
measure url 'http://[a-z]+' Complex
measure uuid '[0-9a-f]+-[0-9a-f]+-[0-9a-f]+-[0-9a-f]+-[0-9a-f]+' Behemoth
measure email '[a-zA-Z0-9._]+@[a-zA-Z0-9.]+[.][a-zA-Z]+' Behemoth

log "Results: $OUT_FILE"
