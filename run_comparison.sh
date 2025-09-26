#!/bin/bash

# CTRE SIMD vs Non-SIMD Comparison Script
# Side-by-side comparison of performance with detailed analysis

set -e

echo "🚀 CTRE SIMD vs Non-SIMD Performance Comparison"
echo "==============================================="
echo

# Clean and build
echo "📦 Building both versions..."
make clean > /dev/null 2>&1
make simd_vs_nosimd_benchmark > /dev/null 2>&1
make simd_vs_nosimd_benchmark_disabled > /dev/null 2>&1

echo "🏃 Running benchmarks..."
echo

# Run both versions and capture output
echo "Running SIMD version..."
SIMD_OUTPUT=$(./tests/simd_vs_nosimd_benchmark)

echo "Running non-SIMD version..."
NOSIMD_OUTPUT=$(./tests/simd_vs_nosimd_benchmark_disabled)

echo
echo "============================================================"
echo "  SIDE-BY-SIDE PERFORMANCE COMPARISON"
echo "============================================================"
echo

# Function to extract timing data
extract_timings() {
    local output="$1"
    local section="$2"
    
    echo "$output" | sed -n "/$section/,/^$/p" | grep "|" | grep -v "Pattern" | grep -v "\-\-\-" | while read line; do
        if [[ $line =~ \|([^|]+)\|([^|]+)\|([^|]+)\| ]]; then
            pattern="${BASH_REMATCH[1]// /}"
            length="${BASH_REMATCH[2]// /}"
            time="${BASH_REMATCH[3]// /}"
            echo "$pattern|$length|$time"
        fi
    done
}

# Create temporary files for comparison
extract_timings "$SIMD_OUTPUT" "SINGLE CHARACTER REPETITION PATTERNS" > /tmp/simd_single.txt
extract_timings "$SIMD_OUTPUT" "CHARACTER CLASS REPETITION PATTERNS" > /tmp/simd_class.txt
extract_timings "$SIMD_OUTPUT" "SMALL RANGE PATTERNS" > /tmp/simd_small.txt
extract_timings "$SIMD_OUTPUT" "EDGE CASES" > /tmp/simd_edge.txt

extract_timings "$NOSIMD_OUTPUT" "SINGLE CHARACTER REPETITION PATTERNS" > /tmp/nosimd_single.txt
extract_timings "$NOSIMD_OUTPUT" "CHARACTER CLASS REPETITION PATTERNS" > /tmp/nosimd_class.txt
extract_timings "$NOSIMD_OUTPUT" "SMALL RANGE PATTERNS" > /tmp/nosimd_small.txt
extract_timings "$NOSIMD_OUTPUT" "EDGE CASES" > /tmp/nosimd_edge.txt

# Function to compare sections
compare_section() {
    local title="$1"
    local simd_file="$2"
    local nosimd_file="$3"
    
    echo "📊 $title"
    echo "Pattern     | Length | SIMD (ns) | No-SIMD (ns) | Speedup | Status"
    echo "------------|--------|-----------|--------------|--------|--------"
    
    while IFS='|' read -r pattern length simd_time; do
        # Find corresponding non-SIMD entry
        nosimd_line=$(grep "^$pattern|$length|" "$nosimd_file" 2>/dev/null || echo "")
        if [[ -n "$nosimd_line" ]]; then
            nosimd_time=$(echo "$nosimd_line" | cut -d'|' -f3)
            
            # Calculate speedup (higher is better for SIMD)
            if (( $(echo "$nosimd_time > 0" | bc -l 2>/dev/null) )); then
                speedup=$(echo "scale=2; $nosimd_time / $simd_time" | bc -l 2>/dev/null)
                if (( $(echo "$speedup > 1.0" | bc -l 2>/dev/null) )); then
                    status="✅ SIMD faster"
                elif (( $(echo "$speedup < 0.8" | bc -l 2>/dev/null) )); then
                    status="❌ SIMD slower"
                else
                    status="➖ Similar"
                fi
                printf "%-11s | %6s | %9s | %12s | %6.2fx | %s\n" "$pattern" "$length" "$simd_time" "$nosimd_time" "$speedup" "$status"
            fi
        fi
    done < "$simd_file"
    echo
}

# Compare each section
compare_section "SINGLE CHARACTER REPETITION PATTERNS" "/tmp/simd_single.txt" "/tmp/nosimd_single.txt"
compare_section "CHARACTER CLASS REPETITION PATTERNS" "/tmp/simd_class.txt" "/tmp/nosimd_class.txt"
compare_section "SMALL RANGE PATTERNS (≤10 chars)" "/tmp/simd_small.txt" "/tmp/nosimd_small.txt"
compare_section "EDGE CASES" "/tmp/simd_edge.txt" "/tmp/nosimd_edge.txt"

# Summary analysis
echo "============================================================"
echo "  PERFORMANCE ANALYSIS SUMMARY"
echo "============================================================"

# Count improvements and regressions by analyzing the output
simd_faster=0
simd_slower=0
similar=0

# Count from all comparison files
for file in /tmp/simd_*.txt; do
    if [ -f "$file" ]; then
        while IFS='|' read -r pattern length simd_time; do
            nosimd_file=$(echo "$file" | sed 's/simd_/nosimd_/')
            if [ -f "$nosimd_file" ]; then
                nosimd_line=$(grep "^$pattern|$length|" "$nosimd_file" 2>/dev/null || echo "")
                if [[ -n "$nosimd_line" ]]; then
                    nosimd_time=$(echo "$nosimd_line" | cut -d'|' -f3)
                    if (( $(echo "$nosimd_time > 0" | bc -l 2>/dev/null) )); then
                        speedup=$(echo "scale=2; $nosimd_time / $simd_time" | bc -l 2>/dev/null)
                        if (( $(echo "$speedup > 1.0" | bc -l 2>/dev/null) )); then
                            ((simd_faster++))
                        elif (( $(echo "$speedup < 0.8" | bc -l 2>/dev/null) )); then
                            ((simd_slower++))
                        else
                            ((similar++))
                        fi
                    fi
                fi
            fi
        done < "$file"
    fi
done

echo "📈 Performance Summary:"
echo "   • SIMD faster: $simd_faster patterns"
echo "   • SIMD slower: $simd_slower patterns" 
echo "   • Similar performance: $similar patterns"
echo

if [ "$simd_slower" -gt "$simd_faster" ]; then
    echo "⚠️  CRITICAL: SIMD implementation is underperforming"
    echo "   Most patterns show SIMD is slower than non-SIMD"
    echo "   This indicates fundamental issues in the SIMD implementation"
    echo
    echo "🔧 Areas needing improvement:"
    echo "   • Single character patterns (a*, a+) - 2-3x slower"
    echo "   • Character class patterns ([0-9]*, [a-z]*) - 2x slower"
    echo "   • Small range patterns ([a-e]*) - 2x slower"
    echo "   • Edge cases - 3-4x slower"
    echo
    echo "💡 Debugging suggestions:"
    echo "   • Check if SIMD code path is actually being taken"
    echo "   • Verify SIMD instructions are being generated"
    echo "   • Profile with 'perf' to see actual instruction counts"
    echo "   • Consider SIMD overhead vs. scalar efficiency"
elif [ "$simd_faster" -gt "$simd_slower" ]; then
    echo "✅ GOOD: SIMD implementation is working well"
    echo "   Most patterns show SIMD improvements"
else
    echo "➖ MIXED: SIMD performance is inconsistent"
    echo "   Some patterns benefit, others don't"
fi

# Clean up
rm -f /tmp/simd_*.txt /tmp/nosimd_*.txt
make clean > /dev/null 2>&1

echo
echo "✅ Side-by-side comparison complete!"
echo "   Use this data to identify specific patterns needing optimization"
