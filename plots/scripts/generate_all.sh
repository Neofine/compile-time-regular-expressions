#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLOTS_DIR="$(dirname "$SCRIPT_DIR")"
cd "$PLOTS_DIR"

if [[ "$1" != "--plots" ]]; then
    echo "Building..."
    ./scripts/build.sh
    echo "Benchmarking..."
    ./scripts/run_benchmarks.sh
fi

echo "Generating plots..."
python3 generate.py

echo "Done: $PLOTS_DIR/output/figures/"
