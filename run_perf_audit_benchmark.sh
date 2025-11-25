#!/bin/bash

# Performance Audit Benchmark
# Tests patterns from Hyperscan paper and verifies correct NFA construction

set -e

echo "=============================================="
echo " PERFORMANCE AUDIT BENCHMARK"
echo "=============================================="
echo

# Build with optimizations
echo "Building performance audit benchmark..."
g++ -std=c++20 -O3 -march=native -mavx2 -msse4.2 -Iinclude \
    tests/perf_audit_benchmark.cpp -o tests/benchmark-exec/perf_audit_benchmark

echo "Running benchmark..."
echo
./tests/benchmark-exec/perf_audit_benchmark

echo
echo "=============================================="
echo " BENCHMARK COMPLETE"
echo "=============================================="
