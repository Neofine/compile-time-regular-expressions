#!/bin/bash
# Setup optimal environment for deterministic benchmarking

set -e

echo "=== CTRE Benchmark Environment Setup ==="
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo "⚠  Not running as root - some optimizations will be skipped"
    echo "   Run with: sudo ./setup_benchmark_env.sh"
    echo ""
fi

# 1. Disable CPU frequency scaling (set to performance mode)
if command -v cpupower &> /dev/null; then
    if [ "$EUID" -eq 0 ]; then
        echo "Setting CPU governor to 'performance' mode..."
        cpupower frequency-set --governor performance > /dev/null 2>&1 || true
        echo "✓ CPU governor set to performance"
    else
        echo "⚠ cpupower requires root - skipping frequency scaling"
    fi
else
    echo "⚠ cpupower not found - install with: sudo apt install linux-tools-generic"
fi

# 2. Disable turbo boost (optional, for extreme stability)
if [ "$EUID" -eq 0 ]; then
    if [ -f /sys/devices/system/cpu/intel_pstate/no_turbo ]; then
        echo 1 > /sys/devices/system/cpu/intel_pstate/no_turbo 2>/dev/null || true
        echo "✓ Intel Turbo Boost disabled"
    elif [ -f /sys/devices/system/cpu/cpufreq/boost ]; then
        echo 0 > /sys/devices/system/cpu/cpufreq/boost 2>/dev/null || true
        echo "✓ CPU boost disabled"
    fi
fi

# 3. Set process priority
echo "Use this script before running benchmarks:"
echo "  sudo ./setup_benchmark_env.sh"
echo ""
echo "To run benchmark with high priority:"
echo "  sudo nice -n -20 ./tests/deterministic_benchmark"
echo ""

# 4. Show current CPU frequency info
echo "Current CPU frequency info:"
if command -v cpupower &> /dev/null; then
    cpupower frequency-info --policy 2>/dev/null | grep -E "(current|governor|available)" || true
else
    cat /proc/cpuinfo | grep "MHz" | head -5 || true
fi

echo ""
echo "=== Setup Complete ==="
echo ""
echo "To restore normal CPU behavior after benchmarking:"
echo "  sudo cpupower frequency-set --governor schedutil"
echo "  echo 0 | sudo tee /sys/devices/system/cpu/intel_pstate/no_turbo"



