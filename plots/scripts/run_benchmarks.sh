#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLOTS_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PLOTS_DIR/build"
OUTPUT_DIR="$PLOTS_DIR/output"
LIB_DIR="$PLOTS_DIR/lib"

export LD_LIBRARY_PATH="$LIB_DIR/lib:$LD_LIBRARY_PATH"

REBUILD=false
CATEGORY="all"

for arg in "$@"; do
    case $arg in
        --rebuild) REBUILD=true ;;
        simple|complex|scaling|realworld) CATEGORY=$arg ;;
    esac
done

[[ "$REBUILD" == "true" || ! -f "$BUILD_DIR/bench_simd" ]] && "$SCRIPT_DIR/build.sh"

mkdir -p "$OUTPUT_DIR"/{simple,complex,scaling,realworld}

run_cat() {
    echo "Running $1..."
    "$BUILD_DIR/bench_simd" "$1" > "$OUTPUT_DIR/$1/simd.csv" 2>/dev/null
    "$BUILD_DIR/bench_baseline" "$1" > "$OUTPUT_DIR/$1/baseline.csv" 2>/dev/null
}

case "$CATEGORY" in
    all) for c in simple complex scaling realworld; do run_cat "$c"; done ;;
    *) run_cat "$CATEGORY" ;;
esac

echo "Output: $OUTPUT_DIR/"
