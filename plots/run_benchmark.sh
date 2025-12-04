#!/bin/bash
# Run the complete benchmark suite
set -e

cd "$(dirname "$0")/benchmark"

echo "Building benchmarks..."
./build.sh

echo ""
echo "Running SIMD benchmark..."
./benchmark_simd > output/simd.csv 2>&1

echo "Running baseline benchmark..."
./benchmark_baseline > output/baseline.csv 2>&1

echo "Combining results..."
cat output/simd.csv > ../output/results.csv
tail -n +2 output/baseline.csv >> ../output/results.csv

echo "Generating plots..."
python3 plot.py ../output/results.csv ../output/

echo ""
echo "✅ Done! Results in plots/output/"
