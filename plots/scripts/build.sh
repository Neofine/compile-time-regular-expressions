#!/bin/bash
# Build script for thesis benchmarks with all engines
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLOTS_DIR="$(dirname "$SCRIPT_DIR")"
PROJECT_ROOT="$(dirname "$PLOTS_DIR")"
BUILD_DIR="$PLOTS_DIR/build"
SRC_DIR="$PLOTS_DIR/benchmarks"
LIB_DIR="$PLOTS_DIR/lib"

CXX="${CXX:-g++}"
CXXFLAGS="-std=c++20 -O3 -march=native -I$PROJECT_ROOT/include -I$SRC_DIR -I$LIB_DIR/include"
LDFLAGS="-L$LIB_DIR/lib -lre2 -lpcre2-8 -lhs -Wl,-rpath,$LIB_DIR/lib"

mkdir -p "$BUILD_DIR"

echo "Building benchmarks with all engines..."
echo "  CXX: $CXX"
echo "  Include: $LIB_DIR/include"
echo "  Libs: $LIB_DIR/lib"
echo ""

# SIMD version
echo "  [1/2] Building SIMD version..."
$CXX $CXXFLAGS "$SRC_DIR/thesis_benchmark.cpp" -o "$BUILD_DIR/bench_simd" $LDFLAGS 2>&1

# Baseline version
echo "  [2/2] Building baseline version..."
$CXX $CXXFLAGS -DCTRE_DISABLE_SIMD "$SRC_DIR/thesis_benchmark.cpp" -o "$BUILD_DIR/bench_baseline" $LDFLAGS 2>&1

echo ""
echo "Done!"
ls -la "$BUILD_DIR"/bench_*
