#!/bin/bash
# Shared functions for benchmark scripts

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[1]}")" && pwd)"
BENCH_DIR="$(dirname "$SCRIPT_DIR")"
PROJECT_ROOT="$(dirname "$BENCH_DIR")"
BUILD_DIR="$BENCH_DIR/build"
OUTPUT_DIR="$BENCH_DIR/output"
CONFIG_FILE="$BENCH_DIR/.benchmark_config"
LIB_DIR="$BENCH_DIR/lib"

log()  { echo "[*] $*"; }
warn() { echo "[!] $*" >&2; }
err()  { echo "[ERROR] $*" >&2; exit 1; }

require_root() {
    [[ $EUID -eq 0 ]] || err "Run with sudo"
}

load_config() {
    if [[ -f "$CONFIG_FILE" ]]; then
        ISOLATE_CPUS=$(grep "^ISOLATE_CPUS=" "$CONFIG_FILE" | cut -d= -f2- || echo "0")
        NUM_NODES=$(grep "^NUM_NODES=" "$CONFIG_FILE" | cut -d= -f2- || echo "1")
    else
        ISOLATE_CPUS="0"
        NUM_NODES=1
    fi
}

save_config() {
    cat > "$CONFIG_FILE" << EOF
ISOLATE_CPUS=$ISOLATE_CPUS
NUM_NODES=$NUM_NODES
TOTAL_CPUS=$(nproc)
EOF
}

check_governor() {
    local gov=$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor 2>/dev/null || echo "unknown")
    [[ "$gov" == "performance" ]]
}

check_aslr() {
    local aslr=$(cat /proc/sys/kernel/randomize_va_space 2>/dev/null || echo "2")
    [[ "$aslr" == "0" ]]
}

set_governor_performance() {
    for cpu in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do
        [[ -f "$cpu" ]] && echo "performance" > "$cpu" 2>/dev/null || true
    done
}

disable_turbo() {
    if [[ -f /sys/devices/system/cpu/intel_pstate/no_turbo ]]; then
        echo 1 > /sys/devices/system/cpu/intel_pstate/no_turbo
    elif [[ -f /sys/devices/system/cpu/cpufreq/boost ]]; then
        echo 0 > /sys/devices/system/cpu/cpufreq/boost 2>/dev/null || true
    fi
}

disable_aslr() {
    if [[ -f /proc/sys/kernel/randomize_va_space ]]; then
        local old=$(cat /proc/sys/kernel/randomize_va_space)
        echo 0 > /proc/sys/kernel/randomize_va_space
        echo "ASLR_OLD=$old" >> "$CONFIG_FILE"
    fi
}

restore_aslr() {
    if [[ -f "$CONFIG_FILE" ]] && grep -q "ASLR_OLD=" "$CONFIG_FILE"; then
        local old=$(grep "ASLR_OLD=" "$CONFIG_FILE" | cut -d= -f2)
        echo "$old" > /proc/sys/kernel/randomize_va_space 2>/dev/null || true
    else
        echo 2 > /proc/sys/kernel/randomize_va_space 2>/dev/null || true
    fi
}

detect_isolated_cpus() {
    local isolated=$(cat /proc/cmdline 2>/dev/null | grep -o 'isolcpus=[0-9,-]*' | cut -d= -f2 || echo "")
    if [[ -n "$isolated" ]]; then
        echo "$isolated"
    elif [[ $(nproc) -ge 4 ]]; then
        echo "$(($(nproc)-2)),$(($(nproc)-1))"
    else
        echo "0"
    fi
}

detect_numa_node() {
    local cpu="$1"
    numactl --hardware 2>/dev/null | grep "cpus:.*$cpu" | head -1 | grep -o "node [0-9]" | grep -o "[0-9]" | head -1 || echo "0"
}

run_isolated() {
    local cmd=("$@")
    load_config
    
    if command -v numactl &>/dev/null && [[ "$ISOLATE_CPUS" != "0" ]]; then
        local node=$(detect_numa_node "${ISOLATE_CPUS%%,*}")
        numactl --membind="$node" --physcpubind="$ISOLATE_CPUS" "${cmd[@]}"
    elif [[ "$ISOLATE_CPUS" != "0" ]]; then
        taskset -c "$ISOLATE_CPUS" "${cmd[@]}"
    else
        "${cmd[@]}"
    fi
}

