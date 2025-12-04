#!/bin/bash

cd /root/compile-time-regular-expressions

echo "Compiling and running individual benchmarks..."
echo "Each binary will be small (< 32KB) to avoid I-cache issues"
echo ""

# Create results directory
mkdir -p results/individual
rm -f results/individual/all_results.txt

# Compile and run each benchmark
total=0
success=0

for bench_file in tests/individual_benchmarks/*_bench.cpp; do
    total=$((total + 1))
    name=$(basename "$bench_file" _bench.cpp)

    # Compile SIMD version
    if g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 -flto \
        "$bench_file" -o "/tmp/${name}_simd" 2>/dev/null; then

        # Compile NoSIMD version
        if g++ -std=c++20 -Iinclude -O3 -DCTRE_DISABLE_SIMD \
            "$bench_file" -o "/tmp/${name}_nosimd" 2>/dev/null; then

            # Run both and capture results (format: name@@pattern@@time@@description)
            simd_result=$(/tmp/${name}_simd)
            nosimd_result=$(/tmp/${name}_nosimd)

            # Parse results - time is field 3 with @@ delimiter
            simd_time=$(echo "$simd_result" | cut -d'@' -f5)
            nosimd_time=$(echo "$nosimd_result" | cut -d'@' -f5)

            # Validate we got numbers
            if [[ "$simd_time" =~ ^[0-9.]+$ ]] && [[ "$nosimd_time" =~ ^[0-9.]+$ ]]; then
                # Calculate speedup
                speedup=$(echo "scale=2; $nosimd_time / $simd_time" | bc 2>/dev/null)

                if [[ -n "$speedup" ]]; then
                    echo "${name}|${simd_time}|${nosimd_time}|${speedup}" >> results/individual/all_results.txt
                    success=$((success + 1))
                    printf "\r[%d/%d] %s: %sx (SIMD: %s ns, NoSIMD: %s ns)    " $success $total "$name" "$speedup" "$simd_time" "$nosimd_time"
                fi
            fi
        fi
    fi
done

echo ""
echo ""
echo "Results saved to results/individual/all_results.txt"
echo "Successfully benchmarked: $success / $total patterns"
echo ""

if [ ! -f results/individual/all_results.txt ]; then
    echo "No results generated!"
    exit 1
fi

# Display summary
echo "============================================"
echo "Top 15 SIMD Speedups:"
echo "============================================"
sort -t'|' -k4 -rn results/individual/all_results.txt | head -15 | \
awk -F'|' '{printf "%-35s SIMD: %8.2f ns  NoSIMD: %8.2f ns  Speedup: %5.2fx\n", $1, $2, $3, $4}'

echo ""
echo "============================================"
echo "Bottom 10 (worst speedups / regressions):"
echo "============================================"
sort -t'|' -k4 -n results/individual/all_results.txt | head -10 | \
awk -F'|' '{printf "%-35s SIMD: %8.2f ns  NoSIMD: %8.2f ns  Speedup: %5.2fx\n", $1, $2, $3, $4}'

# Calculate overall speedup
echo ""
echo "============================================"
awk -F'|' '{
    simd_sum += $2
    nosimd_sum += $3
    count++
}
END {
    if (simd_sum > 0) {
        overall = nosimd_sum / simd_sum
        printf "Overall Average Speedup: %.2fx (%d patterns)\n", overall, count
        printf "Total SIMD time: %.2f ns, Total NoSIMD time: %.2f ns\n", simd_sum, nosimd_sum
    }
}' results/individual/all_results.txt
echo "============================================"
