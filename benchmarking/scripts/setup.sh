#!/bin/bash
# Setup system for reproducible benchmarks (requires root)

source "$(dirname "$0")/common.sh"
require_root

log "Setting up benchmark environment"

ISOLATE_CPUS=$(detect_isolated_cpus)
NUM_NODES=$(numactl --hardware 2>/dev/null | grep -c "node [0-9]" || echo "1")

log "Isolated CPUs: $ISOLATE_CPUS"
log "NUMA nodes: $NUM_NODES"

log "Setting CPU governor to performance"
set_governor_performance

log "Disabling turbo boost"
disable_turbo

log "Disabling ASLR"
disable_aslr

# Move IRQs away from isolated CPUs
if [[ "$ISOLATE_CPUS" != "0" ]] && ! echo ",$ISOLATE_CPUS," | grep -q ",0,"; then
    log "Moving IRQs to CPU 0"
    for irq in /proc/irq/*/smp_affinity_list; do
        [[ -f "$irq" ]] && [[ -w "$irq" ]] && echo "0" > "$irq" 2>/dev/null || true
    done
fi

save_config

log "Verification:"
check_governor && echo "  ✓ Governor: performance" || echo "  ✗ Governor: $(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor 2>/dev/null)"
check_aslr && echo "  ✓ ASLR: disabled" || echo "  ✗ ASLR: enabled"

log "Setup complete. Run: bash scripts/run_paper_benchmarks.sh"
