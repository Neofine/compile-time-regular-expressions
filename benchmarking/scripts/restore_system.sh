#!/bin/bash
# Restore system settings after benchmarking
# Restores ASLR and other settings that were changed

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLOTS_DIR="$(dirname "$SCRIPT_DIR")"
CONFIG_FILE="$PLOTS_DIR/.benchmark_config"

if [ ! -f "$CONFIG_FILE" ]; then
    echo "No benchmark configuration found. Nothing to restore."
    exit 0
fi

echo "=========================================="
echo "Restoring System Settings"
echo "=========================================="
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then 
    echo "WARNING: Some restorations require root access"
    echo "Run with: sudo bash $0"
fi

# Restore ASLR
if [ -f /proc/sys/kernel/randomize_va_space ]; then
    if [ -f "$CONFIG_FILE" ] && grep -q "ASLR_OLD=" "$CONFIG_FILE"; then
        ASLR_OLD=$(grep "ASLR_OLD=" "$CONFIG_FILE" | cut -d= -f2)
        if [ -n "$ASLR_OLD" ]; then
            if [ "$EUID" -eq 0 ]; then
                echo "$ASLR_OLD" > /proc/sys/kernel/randomize_va_space
                ASLR_NEW=$(cat /proc/sys/kernel/randomize_va_space)
                echo "✓ ASLR restored to $ASLR_NEW (was $ASLR_OLD)"
            else
                echo "  ASLR restore requires root: echo $ASLR_OLD > /proc/sys/kernel/randomize_va_space"
            fi
        fi
    else
        # Default restore to 2 (full ASLR)
        if [ "$EUID" -eq 0 ]; then
            echo 2 > /proc/sys/kernel/randomize_va_space
            echo "✓ ASLR restored to 2 (default)"
        else
            echo "  ASLR restore requires root: echo 2 > /proc/sys/kernel/randomize_va_space"
        fi
    fi
fi

echo ""
echo "=========================================="
echo "Restore Complete"
echo "=========================================="
echo ""
echo "Note: CPU governor and turbo boost settings persist until reboot"
echo "      or until changed manually."

