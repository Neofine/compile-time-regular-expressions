#!/bin/bash
# Setup script for publication-quality benchmarks
# Requires root access for CPU isolation and frequency control

set -euo pipefail  # Strict mode: exit on error, undefined vars, pipe failures

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLOTS_DIR="$(dirname "$SCRIPT_DIR")"

echo "=========================================="
echo "Publication-Quality Benchmark Setup"
echo "=========================================="
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then 
    echo "ERROR: This script requires root access for CPU isolation and frequency control"
    echo "Please run with: sudo bash $0"
    exit 1
fi

# Detect available CPUs
TOTAL_CPUS=$(nproc)
echo "Detected $TOTAL_CPUS CPUs"

# Find isolated CPUs (if any)
ISOLATED_CPUS=$(cat /proc/cmdline | grep -o 'isolcpus=[0-9,-]*' | cut -d= -f2 || echo "")
if [ -n "$ISOLATED_CPUS" ]; then
    echo "Found isolated CPUs in kernel cmdline: $ISOLATED_CPUS"
    ISOLATE_CPUS="$ISOLATED_CPUS"
else
    # Use last 2 CPUs for isolation (if >= 4 CPUs)
    if [ "$TOTAL_CPUS" -ge 4 ]; then
        LAST_CPU=$((TOTAL_CPUS - 1))
        SECOND_LAST=$((TOTAL_CPUS - 2))
        ISOLATE_CPUS="$SECOND_LAST,$LAST_CPU"
        echo "Will isolate CPUs: $ISOLATE_CPUS (add isolcpus=$ISOLATE_CPUS to kernel cmdline for permanent isolation)"
    else
        echo "WARNING: System has < 4 CPUs. Consider isolating CPUs manually."
        ISOLATE_CPUS="0"
    fi
fi

# Detect NUMA topology
if command -v numactl &> /dev/null; then
    echo ""
    echo "NUMA Topology:"
    numactl --hardware
    NUM_NODES=$(numactl --hardware | grep -c "node [0-9]" || echo "1")
    echo "Detected $NUM_NODES NUMA node(s)"
else
    echo "WARNING: numactl not found. Install with: apt-get install numactl"
    NUM_NODES=1
fi

echo ""
echo "=========================================="
echo "CPU Frequency and Power Management"
echo "=========================================="

# Set CPU governor to performance
echo "Setting CPU frequency governor to 'performance'..."
set +e
for cpu in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do
    [ -f "$cpu" ] && echo "performance" > "$cpu" 2>/dev/null || true
done
set -e

# Disable turbo boost
if [ -f /sys/devices/system/cpu/intel_pstate/no_turbo ]; then
    echo "Disabling Intel Turbo Boost..."
    echo 1 > /sys/devices/system/cpu/intel_pstate/no_turbo
    echo "  Turbo boost disabled"
elif [ -f /sys/devices/system/cpu/cpufreq/boost ]; then
    echo "Disabling CPU boost..."
    echo 0 > /sys/devices/system/cpu/cpufreq/boost 2>/dev/null || echo "  Cannot disable boost (may not be supported)"
else
    echo "  Turbo boost control not available (may use different mechanism)"
fi

# Set CPU frequency to maximum (if available)
echo "Setting CPUs to maximum frequency..."
for cpu in /sys/devices/system/cpu/cpu*/cpufreq/scaling_max_freq; do
    if [ -f "$cpu" ]; then
        MAX_FREQ=$(cat "$cpu")
        echo "$MAX_FREQ" > "$cpu" 2>/dev/null || true
    fi
done

# Disable ASLR for consistent memory layout
echo ""
echo "Disabling ASLR (Address Space Layout Randomization)..."
if [ -f /proc/sys/kernel/randomize_va_space ]; then
    ASLR_OLD=$(cat /proc/sys/kernel/randomize_va_space)
    echo 0 > /proc/sys/kernel/randomize_va_space
    ASLR_NEW=$(cat /proc/sys/kernel/randomize_va_space)
    if [ "$ASLR_NEW" = "0" ]; then
        echo "  ASLR disabled (was $ASLR_OLD)"
        # Save original value for restoration
        echo "ASLR_OLD=$ASLR_OLD" >> "$PLOTS_DIR/.benchmark_config"
        echo "  NOTE: Original value saved. Restore with: echo $ASLR_OLD > /proc/sys/kernel/randomize_va_space"
    else
        echo "  WARNING: Could not disable ASLR"
    fi
else
    echo "  ASLR control not available"
fi

# Move IRQs away from isolated CPUs (if isolated CPUs are set)
if [ -n "$ISOLATE_CPUS" ] && [ "$ISOLATE_CPUS" != "0" ]; then
    echo ""
    echo "Configuring IRQ affinity (moving IRQs away from isolated CPUs)..."
    
    # Get non-isolated CPUs (simple approach: use CPU 0 if available, else first non-isolated)
    # For simplicity, just use CPU 0 if it's not isolated, otherwise skip IRQ moving
    if ! echo ",$ISOLATE_CPUS," | grep -q ",0,"; then
        set +e
        IRQ_MOVED=0
        IRQ_FAILED=0
        for irq in /proc/irq/*/smp_affinity_list; do
            [ -f "$irq" ] && [ -w "$irq" ] || continue
            if echo "0" > "$irq" 2>/dev/null; then
                IRQ_MOVED=$((IRQ_MOVED + 1))
            else
                IRQ_FAILED=$((IRQ_FAILED + 1))
            fi
        done
        set -e
        echo "  IRQs moved to CPU 0 (Moved: $IRQ_MOVED, Failed: $IRQ_FAILED)"
    else
        echo "  Skipping IRQ move (CPU 0 is isolated)"
    fi
fi

# Check and document SMT/Hyperthreading status
echo ""
echo "SMT/Hyperthreading Status:"
if [ -d /sys/devices/system/cpu/cpu0/topology ]; then
    SMT_ENABLED=false
    for cpu in /sys/devices/system/cpu/cpu*/topology/thread_siblings_list; do
        if [ -f "$cpu" ]; then
            THREADS=$(cat "$cpu" | tr ',' '\n' | wc -l)
            if [ "$THREADS" -gt 1 ]; then
                SMT_ENABLED=true
                break
            fi
        fi
    done
    if [ "$SMT_ENABLED" = true ]; then
        echo "  SMT/Hyperthreading: ENABLED"
        echo "  WARNING: Consider disabling SMT in BIOS for more consistent results"
    else
        echo "  SMT/Hyperthreading: DISABLED"
    fi
else
    echo "  SMT status: Unknown"
fi

# Check transparent huge pages
echo ""
echo "Transparent Huge Pages (THP):"
if [ -f /sys/kernel/mm/transparent_hugepage/enabled ]; then
    THP_STATUS=$(cat /sys/kernel/mm/transparent_hugepage/enabled | grep -o '\[.*\]')
    echo "  Status: $THP_STATUS"
    echo "  NOTE: THP can cause variability. Consider: echo never > /sys/kernel/mm/transparent_hugepage/enabled"
else
    echo "  THP control not available"
fi

echo ""
echo "=========================================="
echo "System Configuration Summary"
echo "=========================================="
echo "Isolated CPUs: $ISOLATE_CPUS"
echo "NUMA nodes: $NUM_NODES"
echo ""
echo "Current CPU frequencies:"
for cpu in /sys/devices/system/cpu/cpu*/cpufreq/scaling_cur_freq; do
    [ -f "$cpu" ] && echo "  $(dirname "$cpu" | xargs basename): $(($(cat "$cpu") / 1000)) MHz" || true
done

# Verify settings
echo ""
echo "Verification:"
echo "-------------"

# Check governor
FIRST_GOV=$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor 2>/dev/null || echo "unknown")
if [ "$FIRST_GOV" = "performance" ]; then
    echo "  ✓ CPU governor: performance"
else
    echo "  ✗ CPU governor: $FIRST_GOV"
fi

# Check turbo boost
if [ -f /sys/devices/system/cpu/intel_pstate/no_turbo ]; then
    TURBO=$(cat /sys/devices/system/cpu/intel_pstate/no_turbo)
    if [ "$TURBO" = "1" ]; then
        echo "  ✓ Turbo boost: DISABLED"
    else
        echo "  ✗ Turbo boost: ENABLED"
    fi
elif [ -f /sys/devices/system/cpu/cpufreq/boost ]; then
    BOOST=$(cat /sys/devices/system/cpu/cpufreq/boost)
    if [ "$BOOST" = "0" ]; then
        echo "  ✓ CPU boost: DISABLED"
    else
        echo "  ✗ CPU boost: ENABLED"
    fi
fi

# Check ASLR
if [ -f /proc/sys/kernel/randomize_va_space ]; then
    ASLR=$(cat /proc/sys/kernel/randomize_va_space)
    if [ "$ASLR" = "0" ]; then
        echo "  ✓ ASLR: DISABLED"
    else
        echo "  ✗ ASLR: ENABLED ($ASLR)"
    fi
fi

echo ""
echo "=========================================="
echo "Recommended Kernel Parameters"
echo "=========================================="
echo "For permanent CPU isolation, add to GRUB_CMDLINE_LINUX in /etc/default/grub:"
echo "  isolcpus=$ISOLATE_CPUS nohz_full=$ISOLATE_CPUS rcu_nocbs=$ISOLATE_CPUS"
echo ""
echo "Then run: sudo update-grub && sudo reboot"
echo ""
echo "For temporary isolation (current session only):"
echo "  Use: taskset -c $ISOLATE_CPUS <command>"
echo "  Or: numactl --physcpubind=$ISOLATE_CPUS <command>"

echo ""
echo "=========================================="
echo "Setup Complete"
echo "=========================================="
echo "Configuration saved to: $PLOTS_DIR/.benchmark_config"
cat > "$PLOTS_DIR/.benchmark_config" << EOF
# Benchmark configuration
ISOLATE_CPUS=$ISOLATE_CPUS
NUM_NODES=$NUM_NODES
TOTAL_CPUS=$TOTAL_CPUS
EOF

echo "Run benchmarks with: bash scripts/run_paper_benchmarks.sh"

