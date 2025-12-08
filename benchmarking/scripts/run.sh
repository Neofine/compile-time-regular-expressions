#!/bin/bash
# Run benchmarks with CPU isolation and statistical aggregation

source "$(dirname "$0")/common.sh"
load_config

RUNS=${BENCHMARK_RUNS:-5}
CATEGORIES="simple complex scaling realworld small large fallback adversarial instantiation NonMatch"

log "Benchmark runner (${RUNS} runs, CPUs: $ISOLATE_CPUS)"

if ! check_governor || ! check_aslr; then
    warn "System not optimally configured. Run: sudo bash scripts/setup.sh"
fi

run_variant() {
    local exe="$1" category="$2" variant="$3"
    local out_dir="$OUTPUT_DIR/$category"
    local temp="$out_dir/${variant}_runs_$$.csv"
    local final="$out_dir/${variant}.csv"
    
    mkdir -p "$out_dir"
    [[ -f "$exe" ]] || { warn "Missing: $exe"; return 1; }
    
    > "$temp"
    for ((i=1; i<=RUNS; i++)); do
        echo "    Run $i/$RUNS"
        [[ $i -gt 1 ]] && sleep 0.1
        run_isolated "$exe" "$category" >> "$temp" 2>&1 || { rm -f "$temp"; return 1; }
    done
    
    python3 "$BENCH_DIR/stats.py" "$temp" "$final" || { rm -f "$temp"; return 1; }
    rm -f "$temp"
}

for cat in $CATEGORIES; do
    log "Category: $cat"
    run_variant "$BUILD_DIR/bench_simd" "$cat" "simd" || warn "simd failed for $cat"
    run_variant "$BUILD_DIR/bench_scalar" "$cat" "scalar" || warn "scalar failed for $cat"  
    run_variant "$BUILD_DIR/bench_original" "$cat" "original" || warn "original failed for $cat"
done

log "Complete. Results in: $OUTPUT_DIR/"
log "Generate plots: python3 generate.py"
