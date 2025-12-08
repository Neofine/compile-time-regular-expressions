#!/bin/bash
# Run all benchmarks: build, runtime, compile time, code size, and generate plots
set -e

cd "$(dirname "$0")/.."
source scripts/common.sh

log "=== CTRE Benchmark Suite ==="

# Build
log "Building executables..."
bash scripts/build.sh

# Runtime benchmarks
CATEGORIES="Simple Complex Scaling RealWorld NonMatch Small Large Fallback Adversarial Instantiation"
log "Running runtime benchmarks..."
for cat in $CATEGORIES; do
    outdir="output/$(echo $cat | tr '[:upper:]' '[:lower:]')"
    mkdir -p "$outdir"
    log "  $cat"
    ./build/bench_simd "$cat" > "$outdir/simd.csv" 2>/dev/null
    ./build/bench_scalar "$cat" > "$outdir/scalar.csv" 2>/dev/null
    ./build/bench_original "$cat" > "$outdir/original.csv" 2>/dev/null
done

# Compile time
log "Measuring compile time..."
bash scripts/compile_time.sh

# Code size
log "Measuring code size..."
bash scripts/codesize.sh

# Generate plots
log "Generating plots..."
python3 generate.py --category all

log "=== Complete ==="
log "Results in: output/"
log "Plots in: output/figures/"


