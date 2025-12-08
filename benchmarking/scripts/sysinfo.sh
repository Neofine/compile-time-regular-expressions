#!/bin/bash
# Collect system info for paper documentation

source "$(dirname "$0")/common.sh"

OUT="${1:-system_info.txt}"

{
    echo "System Information - $(date)"
    echo ""
    echo "=== CPU ==="
    lscpu | grep -E "^(Model name|CPU\(s\)|Thread|Core|Socket|MHz|Cache)"
    echo ""
    echo "=== Memory ==="
    free -h | head -2
    echo ""
    echo "=== Kernel ==="
    uname -r
    echo ""
    echo "=== Compiler ==="
    ${CXX:-g++} --version | head -1
    echo ""
    echo "=== Libraries ==="
    pkg-config --modversion re2 2>/dev/null && echo "RE2: $(pkg-config --modversion re2)"
    pkg-config --modversion libpcre2-8 2>/dev/null && echo "PCRE2: $(pkg-config --modversion libpcre2-8)"
    pkg-config --modversion libhs 2>/dev/null && echo "Hyperscan: $(pkg-config --modversion libhs)"
    echo ""
    echo "=== Benchmark Config ==="
    check_governor && echo "Governor: performance" || echo "Governor: $(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor 2>/dev/null)"
    check_aslr && echo "ASLR: disabled" || echo "ASLR: enabled"
    cat /proc/cmdline 2>/dev/null | grep -o 'isolcpus=[0-9,-]*' || echo "isolcpus: none"
} | tee "$OUT"

log "Saved to: $OUT"
