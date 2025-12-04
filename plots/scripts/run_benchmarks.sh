#!/bin/bash
# Run all benchmarks and generate CSV output
#
# Usage:
#   ./scripts/run_benchmarks.sh              # Run all benchmarks
#   ./scripts/run_benchmarks.sh simple       # Run only simple patterns
#   ./scripts/run_benchmarks.sh --rebuild    # Rebuild before running

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLOTS_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PLOTS_DIR/build"
OUTPUT_DIR="$PLOTS_DIR/output/data"
LIB_DIR="$PLOTS_DIR/lib"

# Export library paths
export LD_LIBRARY_PATH="$LIB_DIR/lib:$LD_LIBRARY_PATH"

# Parse arguments
REBUILD=false
CATEGORY="all"

for arg in "$@"; do
    case $arg in
        --rebuild)
            REBUILD=true
            ;;
        simple|complex|scaling|realworld)
            CATEGORY=$arg
            ;;
    esac
done

# Build if needed
if [[ "$REBUILD" == "true" ]] || [[ ! -f "$BUILD_DIR/benchmark_simd" ]]; then
    echo "Building benchmarks..."
    "$SCRIPT_DIR/build.sh"
fi

# Create output directories
mkdir -p "$OUTPUT_DIR"/{simple,complex,scaling,realworld}

# Run benchmarks
run_category() {
    local cat=$1
    echo ""
    echo "=== Running $cat benchmarks ==="

    echo "  SIMD variant..."
    "$BUILD_DIR/benchmark_simd" --category "$cat" > "$OUTPUT_DIR/$cat/simd.csv" 2>/dev/null || \
        "$BUILD_DIR/benchmark_simd" > "$OUTPUT_DIR/$cat/simd.csv"

    echo "  Baseline variant..."
    "$BUILD_DIR/benchmark_baseline" --category "$cat" > "$OUTPUT_DIR/$cat/baseline.csv" 2>/dev/null || \
        "$BUILD_DIR/benchmark_baseline" > "$OUTPUT_DIR/$cat/baseline.csv"

    echo "  -> $OUTPUT_DIR/$cat/"
}

case "$CATEGORY" in
    all)
        run_category "simple"
        run_category "complex"
        run_category "scaling"
        run_category "realworld"
        ;;
    *)
        run_category "$CATEGORY"
        ;;
esac

echo ""
echo "=== Benchmark complete ==="
echo "Output in: $OUTPUT_DIR/"
find "$OUTPUT_DIR" -name "*.csv" -newer "$BUILD_DIR/benchmark_simd" 2>/dev/null | head -20

