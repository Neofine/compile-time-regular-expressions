#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLOTS_DIR="$(dirname "$SCRIPT_DIR")"
PROJECT_ROOT="$(dirname "$PLOTS_DIR")"
BUILD_DIR="$PLOTS_DIR/build"
SRC_DIR="$PLOTS_DIR/benchmarks"
LIB_DIR="$PLOTS_DIR/lib"
ORIG_CTRE="$LIB_DIR/ctre_original/include"

CXX="${CXX:-g++}"
BASE_CXXFLAGS="-std=c++20 -O3 -I$SRC_DIR -I$LIB_DIR/include"
LDFLAGS="-L$LIB_DIR/lib -lre2 -lpcre2-8 -lhs -Wl,-rpath,$LIB_DIR/lib"

mkdir -p "$BUILD_DIR"

echo "Building CTRE-SIMD (full SIMD)..."
$CXX $BASE_CXXFLAGS -march=native -I$PROJECT_ROOT/include \
    "$SRC_DIR/thesis_benchmark.cpp" -o "$BUILD_DIR/bench_simd" $LDFLAGS

echo "Building CTRE-Scalar (SIMD code but scalar fallback only)..."
$CXX $BASE_CXXFLAGS -mno-avx2 -mno-avx -msse4.2 -I$PROJECT_ROOT/include -DCTRE_ENGINE_NAME='"CTRE-Scalar"' \
    "$SRC_DIR/thesis_benchmark.cpp" -o "$BUILD_DIR/bench_scalar" $LDFLAGS 2>/dev/null || \
$CXX $BASE_CXXFLAGS -I$PROJECT_ROOT/include -DCTRE_DISABLE_SIMD -DCTRE_ENGINE_NAME='"CTRE-Scalar"' \
    "$SRC_DIR/thesis_benchmark.cpp" -o "$BUILD_DIR/bench_scalar" $LDFLAGS

echo "Building CTRE-Original (upstream CTRE, no modifications)..."
$CXX $BASE_CXXFLAGS -march=native -I$ORIG_CTRE -DCTRE_ENGINE_NAME='"CTRE"' \
    "$SRC_DIR/thesis_benchmark.cpp" -o "$BUILD_DIR/bench_original" $LDFLAGS

ls -la "$BUILD_DIR"/bench_*
