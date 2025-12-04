#!/bin/bash
# Run master benchmark for quick testing
# Restored from git history reference
set -e

cd "$(dirname "$0")"

echo "=== Building master_benchmark ==="
g++ -std=c++20 -O3 -march=native -Iinclude \
    tests/master_benchmark.cpp \
    -o tests/master_benchmark 2>&1

echo ""
echo "=== Running master_benchmark ==="
./tests/master_benchmark


