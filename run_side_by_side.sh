#!/bin/bash

# CTRE SIMD vs Non-SIMD Side-by-Side Comparison
# Shows results side by side for easy analysis

set -e

echo "🚀 CTRE SIMD vs Non-SIMD Side-by-Side Comparison"
echo "================================================"
echo

# Clean and build
echo "📦 Building both versions..."
make clean > /dev/null 2>&1
make simd_vs_nosimd_benchmark > /dev/null 2>&1
make simd_vs_nosimd_benchmark_disabled > /dev/null 2>&1

echo "🏃 Running benchmarks..."
echo

# Run both versions
echo "Running SIMD version..."
SIMD_OUTPUT=$(./tests/simd_vs_nosimd_benchmark)

echo "Running non-SIMD version..."
NOSIMD_OUTPUT=$(./tests/simd_vs_nosimd_benchmark_disabled)

echo
echo "============================================================"
echo "  SIDE-BY-SIDE PERFORMANCE COMPARISON"
echo "============================================================"
echo

# Function to extract and format data for comparison
extract_data() {
    local output="$1"
    local section="$2"
    
    echo "$output" | sed -n "/$section/,/^$/p" | grep "|" | grep -v "Pattern" | grep -v "---" | while read line; do
        if [[ $line =~ \|([^|]+)\|([^|]+)\|([^|]+)\| ]]; then
            pattern="${BASH_REMATCH[1]// /}"
            length="${BASH_REMATCH[2]// /}"
            time="${BASH_REMATCH[3]// /}"
            echo "$pattern|$length|$time"
        fi
    done
}

# Create comparison data
echo "📊 SINGLE CHARACTER REPETITION PATTERNS"
echo "Pattern     | Length | SIMD (ns) | No-SIMD (ns) | Speedup | Status"
echo "------------|--------|-----------|--------------|--------|--------"

# Extract single character data
SIMD_SINGLE=$(extract_data "$SIMD_OUTPUT" "SINGLE CHARACTER REPETITION PATTERNS")
NOSIMD_SINGLE=$(extract_data "$NOSIMD_OUTPUT" "SINGLE CHARACTER REPETITION PATTERNS")

# Compare single character patterns
echo "$SIMD_SINGLE" | while IFS='|' read -r pattern length simd_time; do
    if [[ -n "$pattern" ]]; then
        nosimd_line=$(echo "$NOSIMD_SINGLE" | grep "^$pattern|$length|" || echo "")
        if [[ -n "$nosimd_line" ]]; then
            nosimd_time=$(echo "$nosimd_line" | cut -d'|' -f3)
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
    fi
done

echo
echo "📊 CHARACTER CLASS REPETITION PATTERNS"
echo "Pattern     | Length | SIMD (ns) | No-SIMD (ns) | Speedup | Status"
echo "------------|--------|-----------|--------------|--------|--------"

# Extract character class data
SIMD_CLASS=$(extract_data "$SIMD_OUTPUT" "CHARACTER CLASS REPETITION PATTERNS")
NOSIMD_CLASS=$(extract_data "$NOSIMD_OUTPUT" "CHARACTER CLASS REPETITION PATTERNS")

# Compare character class patterns
echo "$SIMD_CLASS" | while IFS='|' read -r pattern length simd_time; do
    if [[ -n "$pattern" ]]; then
        nosimd_line=$(echo "$NOSIMD_CLASS" | grep "^$pattern|$length|" || echo "")
        if [[ -n "$nosimd_line" ]]; then
            nosimd_time=$(echo "$nosimd_line" | cut -d'|' -f3)
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
    fi
done

echo
echo "📊 SMALL RANGE PATTERNS (≤10 chars)"
echo "Pattern     | Length | SIMD (ns) | No-SIMD (ns) | Speedup | Status"
echo "------------|--------|-----------|--------------|--------|--------"

# Extract small range data
SIMD_SMALL=$(extract_data "$SIMD_OUTPUT" "SMALL RANGE PATTERNS")
NOSIMD_SMALL=$(extract_data "$NOSIMD_OUTPUT" "SMALL RANGE PATTERNS")

# Compare small range patterns
echo "$SIMD_SMALL" | while IFS='|' read -r pattern length simd_time; do
    if [[ -n "$pattern" ]]; then
        nosimd_line=$(echo "$NOSIMD_SMALL" | grep "^$pattern|$length|" || echo "")
        if [[ -n "$nosimd_line" ]]; then
            nosimd_time=$(echo "$nosimd_line" | cut -d'|' -f3)
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
    fi
done

echo
echo "📊 EDGE CASES"
echo "Pattern     | Length | SIMD (ns) | No-SIMD (ns) | Speedup | Status"
echo "------------|--------|-----------|--------------|--------|--------"

# Extract edge case data
SIMD_EDGE=$(extract_data "$SIMD_OUTPUT" "EDGE CASES")
NOSIMD_EDGE=$(extract_data "$NOSIMD_OUTPUT" "EDGE CASES")

# Compare edge cases
echo "$SIMD_EDGE" | while IFS='|' read -r pattern length simd_time; do
    if [[ -n "$pattern" ]]; then
        nosimd_line=$(echo "$NOSIMD_EDGE" | grep "^$pattern|$length|" || echo "")
        if [[ -n "$nosimd_line" ]]; then
            nosimd_time=$(echo "$nosimd_line" | cut -d'|' -f3)
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
    fi
done

echo
echo "============================================================"
echo "  KEY FINDINGS"
echo "============================================================"
echo "🔍 Analysis:"
echo "   • Look for patterns marked with ❌ SIMD slower"
echo "   • These indicate areas needing optimization"
echo "   • Speedup < 1.0 means SIMD is slower than non-SIMD"
echo "   • Speedup > 1.0 means SIMD is faster (good!)"
echo
echo "💡 Next steps:"
echo "   • Focus on patterns with speedup < 0.5 (2x slower)"
echo "   • Check if SIMD code path is actually being taken"
echo "   • Profile with 'perf' to see instruction counts"
echo "   • Consider SIMD overhead vs. scalar efficiency"
echo

# Clean up
make clean > /dev/null 2>&1

echo "✅ Side-by-side comparison complete!"
