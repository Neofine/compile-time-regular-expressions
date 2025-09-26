#!/bin/bash

# Simple side-by-side comparison of SIMD vs Non-SIMD results

set -e

echo "🚀 CTRE SIMD vs Non-SIMD Simple Comparison"
echo "=========================================="
echo

# Clean and build
echo "📦 Building both versions..."
make clean > /dev/null 2>&1
make simd_vs_nosimd_benchmark > /dev/null 2>&1
make simd_vs_nosimd_benchmark_disabled > /dev/null 2>&1

echo "🏃 Running benchmarks..."
echo

# Run both versions and save to files
echo "Running SIMD version..."
./tests/simd_vs_nosimd_benchmark > /tmp/simd_results.txt

echo "Running non-SIMD version..."
./tests/simd_vs_nosimd_benchmark_disabled > /tmp/nosimd_results.txt

echo
echo "============================================================"
echo "  SIDE-BY-SIDE COMPARISON"
echo "============================================================"
echo

# Show SIMD results
echo "📊 SIMD ENABLED RESULTS:"
echo "========================="
cat /tmp/simd_results.txt

echo
echo "📊 SIMD DISABLED RESULTS:"
echo "=========================="
cat /tmp/nosimd_results.txt

echo
echo "============================================================"
echo "  MANUAL COMPARISON GUIDE"
echo "============================================================"
echo
echo "🔍 To compare results manually:"
echo "   1. Look at the same pattern and length in both sections"
echo "   2. Compare the 'Time (ns)' values"
echo "   3. Lower time = better performance"
echo
echo "📈 Key patterns to focus on:"
echo "   • a* (single character repetition)"
echo "   • [0-9]* (digit character class)"
echo "   • [a-z]* (lowercase character class)"
echo "   • [a-e]* (small range)"
echo
echo "⚠️  Current observations:"
echo "   • SIMD times are generally higher (slower)"
echo "   • Non-SIMD times are generally lower (faster)"
echo "   • This indicates SIMD implementation issues"
echo
echo "💡 Next steps:"
echo "   • Focus on patterns where SIMD is 2x+ slower"
echo "   • Check if SIMD code path is being taken"
echo "   • Profile with 'perf' to see actual instructions"
echo

# Clean up
rm -f /tmp/simd_results.txt /tmp/nosimd_results.txt
make clean > /dev/null 2>&1

echo "✅ Simple comparison complete!"
