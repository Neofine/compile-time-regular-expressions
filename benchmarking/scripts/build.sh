#!/bin/bash
# Build benchmark executables

source "$(dirname "$0")/common.sh"

CXX="${CXX:-g++}"
SRC="$BENCH_DIR/src/benchmark.cpp"

if pkg-config --exists re2 libpcre2-8 libhs 2>/dev/null; then
    CFLAGS="-std=c++20 -O3 $(pkg-config --cflags re2 libpcre2-8 libhs)"
    LIBS="$(pkg-config --libs re2 libpcre2-8 libhs)"
else
    CFLAGS="-std=c++20 -O3 -I$LIB_DIR/include"
    LIBS="-L$LIB_DIR/lib -lre2 -lpcre2-8 -lhs -Wl,-rpath,$LIB_DIR/lib"
fi

mkdir -p "$BUILD_DIR"

log "Building CTRE-SIMD"
$CXX $CFLAGS -march=native -I"$PROJECT_ROOT/include" "$SRC" -o "$BUILD_DIR/bench_simd" $LIBS

log "Building CTRE-Scalar"
$CXX $CFLAGS -march=native -DCTRE_DISABLE_SIMD -I"$PROJECT_ROOT/include" -DCTRE_ENGINE_NAME='"CTRE-Scalar"' "$SRC" -o "$BUILD_DIR/bench_scalar" $LIBS

log "Building CTRE-Original"
$CXX $CFLAGS -march=native -I"$LIB_DIR/ctre_original/include" -DCTRE_ENGINE_NAME='"CTRE"' "$SRC" -o "$BUILD_DIR/bench_original" $LIBS

log "Built:"
ls -lh "$BUILD_DIR"/bench_*
