#!/bin/bash

# Master Benchmark V2 - CTRE vs BitNFA Comparison
# Shows where each engine excels

set -e

echo "=============================================================="
echo " MASTER BENCHMARK V2 - CTRE vs BitNFA"
echo " Comprehensive comparison across input sizes"
echo "=============================================================="
echo ""

echo "Building benchmark..."
g++ -std=c++20 -Iinclude -Isrell_include -O3 -march=native -mavx2 -msse4.2 \
    -funroll-loops -flto \
    tests/master_benchmark_with_bitnfa.cpp -o tests/master_benchmark_with_bitnfa

echo "Running benchmark (this may take a minute)..."
echo ""

./tests/master_benchmark_with_bitnfa

echo ""
echo "=============================================================="
echo " CONCLUSION"
echo "=============================================================="
echo ""
echo "CTRE and BitNFA are COMPLEMENTARY engines:"
echo ""
echo "  ✓ CTRE excels at: Short inputs, validation, parsing"
echo "  ✓ BitNFA excels at: Long inputs, text search, log processing"
echo ""
echo "Both engines are valuable and serve different use cases!"
echo ""

# Cleanup
rm -f tests/master_benchmark_with_bitnfa
