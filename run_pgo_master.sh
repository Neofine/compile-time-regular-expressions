#!/bin/bash
set -e

echo "🎯 PGO for Master Benchmark - Tuned for actual usage patterns"
echo ""

# Step 1: Compile with profiling instrumentation
echo "Step 1: Compiling with profiling..."
g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 -flto \
    -fprofile-generate=/tmp/pgo_data \
    tests/master_benchmark.cpp -o /tmp/bench_profile

# Step 2: Run to collect profile data
echo "Step 2: Collecting profile data (3 runs)..."
for i in 1 2 3; do
    /tmp/bench_profile > /dev/null 2>&1
done

# Step 3: Recompile with profile-guided optimizations
echo "Step 3: Recompiling with PGO..."
g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 -flto \
    -fprofile-use=/tmp/pgo_data -fprofile-correction \
    tests/master_benchmark.cpp -o /tmp/bench_pgo

# Step 4: Compare
echo ""
echo "🏁 Testing PGO-optimized benchmark..."
./run_master_benchmark.sh 2>&1 | grep "Overall speedup"

# Cleanup
rm -rf /tmp/pgo_data
