#!/bin/bash
# Run publication-quality benchmarks with CPU isolation and NUMA pinning

set -euo pipefail  # Strict mode: exit on error, undefined vars, pipe failures

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLOTS_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PLOTS_DIR/build"
OUTPUT_DIR="$PLOTS_DIR/output"
CONFIG_FILE="$PLOTS_DIR/.benchmark_config"

# Load configuration (validate file exists and is readable)
if [ -f "$CONFIG_FILE" ] && [ -r "$CONFIG_FILE" ]; then
    # Use safer parsing instead of source (prevents code injection)
    ISOLATE_CPUS=$(grep "^ISOLATE_CPUS=" "$CONFIG_FILE" | cut -d= -f2- || echo "0")
    NUM_NODES=$(grep "^NUM_NODES=" "$CONFIG_FILE" | cut -d= -f2- || echo "1")
    TOTAL_CPUS=$(grep "^TOTAL_CPUS=" "$CONFIG_FILE" | cut -d= -f2- || echo "$(nproc)")
else
    echo "WARNING: Configuration file not found. Run setup_paper_benchmarks.sh first."
    ISOLATE_CPUS="0"
    NUM_NODES=1
fi

# Number of runs for statistical analysis
RUNS=${BENCHMARK_RUNS:-5}

echo "=========================================="
echo "Publication-Quality Benchmark Runner"
echo "=========================================="
echo "Isolated CPUs: $ISOLATE_CPUS"
echo "NUMA nodes: $NUM_NODES"
echo "Number of runs: $RUNS"
echo ""

# Quick verification
FIRST_GOV=$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor 2>/dev/null || echo "unknown")
ASLR=$(cat /proc/sys/kernel/randomize_va_space 2>/dev/null || echo "unknown")

if [ "$FIRST_GOV" != "performance" ] || [ "$ASLR" != "0" ]; then
    echo "WARNING: System may not be optimally configured"
    echo "  Governor: $FIRST_GOV (expected: performance)"
    echo "  ASLR: $ASLR (expected: 0)"
    echo "  Run 'sudo bash scripts/setup_paper_benchmarks.sh' to fix"
    echo ""
fi

echo ""

# Check if numactl is available
if command -v numactl &> /dev/null; then
    USE_NUMA=true
    # Detect which NUMA node the isolated CPUs are on
    if [ -n "$ISOLATE_CPUS" ] && [ "$ISOLATE_CPUS" != "0" ]; then
        # Get first isolated CPU
        FIRST_ISOLATED_CPU=$(echo "$ISOLATE_CPUS" | cut -d',' -f1 | cut -d'-' -f1)
        # Find which NUMA node this CPU belongs to
        NUMA_NODE=$(numactl --hardware 2>/dev/null | grep "cpus:.*$FIRST_ISOLATED_CPU" | head -1 | grep -o "node [0-9]" | grep -o "[0-9]" | head -1 || echo "0")
        if [ -z "$NUMA_NODE" ]; then
            NUMA_NODE=0
        fi
        echo "Using numactl for NUMA pinning (node $NUMA_NODE, isolated CPUs on this node)"
    else
        NUMA_NODE=0
        echo "Using numactl for NUMA pinning (node $NUMA_NODE)"
    fi
else
    USE_NUMA=false
    echo "numactl not available, using taskset only"
fi

# Function to run benchmark with isolation
run_benchmark() {
    local bench_exe="$1"
    local category="$2"
    local output_file="$3"
    local run_num="$4"
    
    if [ ! -f "$bench_exe" ]; then
        echo "ERROR: Benchmark executable not found: $bench_exe"
        return 1
    fi
    
    # Build command with CPU isolation and high priority
    RT_PRIORITY=${RT_PRIORITY:-5}  # Real-time priority (1-99, lower is safer)
    
    # Validate category name (only alphanumeric, dash, underscore)
    if [[ ! "$category" =~ ^[a-zA-Z0-9_-]+$ ]]; then
        echo "ERROR: Invalid category name: $category" >&2
        return 1
    fi
    
    # Use array-based command execution (safer than eval)
    if [ "$USE_NUMA" = true ]; then
        if command -v chrt &> /dev/null && chrt -f "$RT_PRIORITY" true 2>/dev/null; then
            CMD_ARRAY=(numactl --membind="$NUMA_NODE" --physcpubind="$ISOLATE_CPUS" chrt -f "$RT_PRIORITY" "$bench_exe" "$category")
        else
            CMD_ARRAY=(numactl --membind="$NUMA_NODE" --physcpubind="$ISOLATE_CPUS" nice -n -20 "$bench_exe" "$category")
        fi
    else
        if command -v chrt &> /dev/null && chrt -f "$RT_PRIORITY" true 2>/dev/null; then
            CMD_ARRAY=(taskset -c "$ISOLATE_CPUS" chrt -f "$RT_PRIORITY" "$bench_exe" "$category")
        else
            CMD_ARRAY=(taskset -c "$ISOLATE_CPUS" nice -n -20 "$bench_exe" "$category")
        fi
    fi
    
    # Run and capture output
    if [ -n "$run_num" ]; then
        "${CMD_ARRAY[@]}" >> "$output_file" 2>&1 || return $?
    else
        "${CMD_ARRAY[@]}" > "$output_file" 2>&1 || return $?
    fi
}

# Function to run multiple times and compute statistics
run_with_stats() {
    local bench_exe="$1"
    local category="$2"
    local variant="$3"  # simd, scalar, original
    local output_dir="$OUTPUT_DIR/$category"
    
    mkdir -p "$output_dir" || return 1
    # Use PID in temp file name to avoid race conditions
    local temp_file="$output_dir/${variant}_runs_$$.csv"
    local final_file="$output_dir/${variant}.csv"
    
    echo "  Running $category ($variant) - $RUNS runs..."
    
    # Clear temp file
    > "$temp_file"
    
    # Run multiple times
    for ((i=1; i<=RUNS; i++)); do
        echo "    Run $i/$RUNS..."
        
        # Small delay between runs to let system settle (100ms)
        if [ $i -gt 1 ]; then
            sleep 0.1
        fi
        
        if ! run_benchmark "$bench_exe" "$category" "$temp_file" "$i"; then
            echo "    ERROR: Run $i failed, skipping statistics for this variant"
            rm -f "$temp_file"
            return 1
        fi
        
        # Check if first run produced data
        if [ $i -eq 1 ] && [ ! -s "$temp_file" ]; then
            echo "    ERROR: First run produced no output"
            rm -f "$temp_file"
            return 1
        fi
    done
    
    # Process results to compute statistics
    echo "  Computing statistics..."
    AGGREGATE_SCRIPT="$SCRIPT_DIR/aggregate_stats.py"
    if [ ! -f "$AGGREGATE_SCRIPT" ]; then
        echo "    ERROR: aggregate_stats.py not found: $AGGREGATE_SCRIPT" >&2
        rm -f "$temp_file"
        return 1
    fi
    
    if ! python3 "$AGGREGATE_SCRIPT" "$temp_file" "$final_file"; then
        echo "    ERROR: Statistics computation failed" >&2
        rm -f "$temp_file"
        return 1
    fi
    
    # Clean up temp file
    rm -f "$temp_file"
}

# Main execution
CATEGORIES="simple complex scaling realworld small large fallback adversarial instantiation NonMatch"

echo "Starting benchmarks..."
echo ""

for cat in $CATEGORIES; do
    echo "=========================================="
    echo "Category: $cat"
    echo "=========================================="
    
    # Run with statistics (continue on failure for individual variants)
    set +e  # Don't exit on individual variant failures
    run_with_stats "$BUILD_DIR/bench_simd" "$cat" "simd" || echo "  WARNING: simd variant failed for $cat"
    run_with_stats "$BUILD_DIR/bench_scalar" "$cat" "scalar" || echo "  WARNING: scalar variant failed for $cat"
    run_with_stats "$BUILD_DIR/bench_original" "$cat" "original" || echo "  WARNING: original variant failed for $cat"
    set -e  # Re-enable strict mode
    
    echo ""
done

echo "=========================================="
echo "Benchmarks Complete"
echo "=========================================="
echo "Results in: $OUTPUT_DIR/"
echo "Run: python3 generate.py"
echo ""

