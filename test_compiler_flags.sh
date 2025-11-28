#!/bin/bash
echo "=== TESTING COMPILER FLAG OPTIMIZATIONS ==="
echo ""

pattern="a*_32"

echo "Testing different optimization flags for $pattern:"
echo ""

# Baseline
echo "1. Baseline (-O3 -march=native -mavx2):"
g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 \
    "tests/individual_benchmarks/${pattern}_bench.cpp" -o /tmp/test1 2>/dev/null
result1=$(/tmp/test1 | awk -F'@@' '{print $3}')
echo "   SIMD time: $result1 ns"

# With additional flags
echo ""
echo "2. With -funroll-loops:"
g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 -funroll-loops \
    "tests/individual_benchmarks/${pattern}_bench.cpp" -o /tmp/test2 2>/dev/null
result2=$(/tmp/test2 | awk -F'@@' '{print $3}')
echo "   SIMD time: $result2 ns"
speedup=$(awk -v r1=$result1 -v r2=$result2 'BEGIN{printf "%.1f%%", (r1/r2 - 1)*100}')
echo "   Change: $speedup"

echo ""
echo "3. With -fno-stack-protector (remove canary overhead):"
g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 -fno-stack-protector \
    "tests/individual_benchmarks/${pattern}_bench.cpp" -o /tmp/test3 2>/dev/null
result3=$(/tmp/test3 | awk -F'@@' '{print $3}')
echo "   SIMD time: $result3 ns"
speedup=$(awk -v r1=$result1 -v r3=$result3 'BEGIN{printf "%.1f%%", (r1/r3 - 1)*100}')
echo "   Change: $speedup"

echo ""
echo "4. With -fno-plt (optimize function calls):"
g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 -fno-plt \
    "tests/individual_benchmarks/${pattern}_bench.cpp" -o /tmp/test4 2>/dev/null
result4=$(/tmp/test4 | awk -F'@@' '{print $3}')
echo "   SIMD time: $result4 ns"
speedup=$(awk -v r1=$result1 -v r4=$result4 'BEGIN{printf "%.1f%%", (r1/r4 - 1)*100}')
echo "   Change: $speedup"

echo ""
echo "5. Aggressive (-O3 -march=native -mavx2 -funroll-loops -fno-stack-protector):"
g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 -funroll-loops -fno-stack-protector \
    "tests/individual_benchmarks/${pattern}_bench.cpp" -o /tmp/test5 2>/dev/null
result5=$(/tmp/test5 | awk -F'@@' '{print $3}')
echo "   SIMD time: $result5 ns"
speedup=$(awk -v r1=$result1 -v r5=$result5 'BEGIN{printf "%.1f%%", (r1/r5 - 1)*100}')
echo "   Change: $speedup"

