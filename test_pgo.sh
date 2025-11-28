#!/bin/bash
echo "=== TESTING PROFILE-GUIDED OPTIMIZATION (PGO) ==="
echo ""
echo "Step 1: Compile with profiling instrumentation"
g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 -fprofile-generate tests/master_benchmark.cpp -o /tmp/bench_pgo 2>&1 | head -5

echo "Step 2: Run to generate profile data"
cd /tmp && /tmp/bench_pgo 2>&1 | head -5

echo ""
echo "Step 3: Recompile with profile data"
g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 -fprofile-use -fprofile-correction tests/master_benchmark.cpp -o /tmp/bench_pgo_opt 2>&1 | head -10

echo ""
echo "Step 4: Test performance"
echo "Without PGO:"
/tmp/bench_pgo | grep "Average speedup"

echo ""
echo "With PGO:"
/tmp/bench_pgo_opt | grep "Average speedup"
