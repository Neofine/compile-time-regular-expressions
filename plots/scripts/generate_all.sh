#!/bin/bash
# Full pipeline: build, benchmark, and generate plots
#
# Usage:
#   ./scripts/generate_all.sh           # Full pipeline
#   ./scripts/generate_all.sh --plots   # Only regenerate plots

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLOTS_DIR="$(dirname "$SCRIPT_DIR")"

cd "$PLOTS_DIR"

if [[ "$1" != "--plots" ]]; then
    echo "=== Building benchmarks ==="
    ./scripts/build.sh

    echo ""
    echo "=== Running benchmarks ==="
    ./scripts/run_benchmarks.sh
fi

echo ""
echo "=== Generating plots ==="
python3 generate.py

echo ""
echo "=== Done! ==="
echo "Figures in: $PLOTS_DIR/output/figures/"

