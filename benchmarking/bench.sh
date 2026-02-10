#!/bin/bash
set -e
cd "$(dirname "$0")"

BUILD=false
PERF=false
CATEGORY=""

usage() {
    cat <<EOF
Usage: $0 [OPTIONS] [CATEGORY]

Options:
  -b, --build    Build benchmark executables
  -p, --perf     Enable performance mode (requires sudo)
  -h, --help     Show this help

Categories:
  Simple, Complex, RealWorld, Scaling, NonMatch, Small, Large, 
  Fallback, Adversarial, Instantiation

Examples:
  $0 --build                # Build only
  $0 Simple                 # Run Simple category
  $0 --build Simple         # Build then run
  sudo $0 --perf Simple     # Run with CPU tuning
  $0 > results.csv          # Save all results to CSV
EOF
    exit 0
}

while [[ $# -gt 0 ]]; do
    case $1 in
        -b|--build) BUILD=true; shift ;;
        -p|--perf)  PERF=true; shift ;;
        -h|--help)  usage ;;
        -*)         echo "Unknown option: $1"; usage ;;
        *)          CATEGORY="$1"; shift ;;
    esac
done

do_build() {
    local CXX="${CXX:-g++}"
    local CFLAGS="-std=c++20 -O3 -march=native"
    local LIBS=""

    if pkg-config --exists re2 libpcre2-8 libhs 2>/dev/null; then
        CFLAGS="$CFLAGS $(pkg-config --cflags re2 libpcre2-8 libhs)"
        LIBS="$(pkg-config --libs re2 libpcre2-8 libhs)"
    else
        echo "Note: Install re2, pcre2, hyperscan for comparisons" >&2
        LIBS="-lre2 -lpcre2-8 -lhs"
    fi

    mkdir -p build
    echo "Building benchmarks..." >&2
    $CXX $CFLAGS -I../include src/benchmark.cpp -o build/bench_simd $LIBS
    $CXX $CFLAGS -DCTRE_DISABLE_SIMD -DCTRE_ENGINE_NAME='"CTRE-Scalar"' -I../include src/benchmark.cpp -o build/bench_scalar $LIBS
    $CXX $CFLAGS -Ilib/ctre_original/include -DCTRE_ENGINE_NAME='"CTRE"' src/benchmark.cpp -o build/bench_original $LIBS
    echo "Done." >&2
}

run_bench() {
    local exe="$1" cat="$2"
    if [[ -f "$exe" ]]; then
        "$exe" $cat 2>/dev/null | tail -n +2  # Skip header (already printed by first)
    fi
}

do_run() {
    [[ -f build/bench_simd ]] || { echo "Error: Run with --build first" >&2; exit 1; }

    # Save original settings for restoration
    local ASLR_OLD=""
    local GOV_OLD=""

    if $PERF; then
        [[ $EUID -eq 0 ]] || { echo "Error: --perf requires sudo" >&2; exit 1; }
        
        ASLR_OLD=$(cat /proc/sys/kernel/randomize_va_space 2>/dev/null || echo "2")
        echo 0 > /proc/sys/kernel/randomize_va_space 2>/dev/null || true
        
        GOV_OLD=$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor 2>/dev/null || echo "")
        for gov in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do
            [[ -f "$gov" ]] && echo "performance" > "$gov" 2>/dev/null || true
        done
        
        [[ -f /sys/devices/system/cpu/intel_pstate/no_turbo ]] && echo 1 > /sys/devices/system/cpu/intel_pstate/no_turbo 2>/dev/null || true
        [[ -f /sys/devices/system/cpu/cpufreq/boost ]] && echo 0 > /sys/devices/system/cpu/cpufreq/boost 2>/dev/null || true
    fi

    # Run all variants and combine (SIMD first for header)
    ./build/bench_simd $CATEGORY
    run_bench ./build/bench_scalar "$CATEGORY"
    run_bench ./build/bench_original "$CATEGORY"

    if $PERF; then
        [[ -n "$ASLR_OLD" ]] && echo "$ASLR_OLD" > /proc/sys/kernel/randomize_va_space 2>/dev/null || true
        if [[ -n "$GOV_OLD" ]]; then
            for gov in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do
                [[ -f "$gov" ]] && echo "$GOV_OLD" > "$gov" 2>/dev/null || true
            done
        fi
    fi
}

# Main execution logic
if $BUILD; then
    do_build
fi

if [[ -n "$CATEGORY" ]] || ! $BUILD; then
    do_run
fi
