#!/bin/bash

echo "🔍 Single Character Pattern Analysis"
echo "===================================="
echo ""

echo "📊 Running existing benchmarks to compare single character vs character class patterns..."
echo ""

# Run character class benchmark
echo "🏃 Running character class benchmark..."
./tests/simd_character_class_benchmark > /tmp/char_class_results.txt 2>&1

# Run single character benchmark  
echo "🏃 Running single character benchmark..."
./tests/simd_repetition_benchmark > /tmp/single_char_results.txt 2>&1

echo ""
echo "📈 Performance Comparison:"
echo "========================="
echo ""

# Extract key results from character class benchmark
echo "Character Class Patterns ([0-9]*, [a-z]*, etc.):"
echo "------------------------------------------------"
grep "Pattern:" /tmp/char_class_results.txt | head -5

echo ""
echo "Single Character Patterns (a*, a+, etc.):"
echo "-----------------------------------------"
grep "Testing pattern:" /tmp/single_char_results.txt | head -5

echo ""
echo "🔍 Analysis:"
echo "============"
echo ""

# Check if single character patterns are working
if grep -q "a\* against 32-character string" /tmp/single_char_results.txt; then
    echo "✅ Single character patterns (a*) are working"
else
    echo "❌ Single character patterns (a*) are NOT working"
fi

# Check if character class patterns are working  
if grep -q "Pattern: \[0-9\]\*" /tmp/char_class_results.txt; then
    echo "✅ Character class patterns ([0-9]*) are working"
else
    echo "❌ Character class patterns ([0-9]*) are NOT working"
fi

echo ""
echo "📊 Performance Summary:"
echo "======================"
echo ""

# Extract timing information
echo "Character Class Performance (SIMD enabled):"
grep "Pattern: \[0-9\]\*.*String length: 32" /tmp/char_class_results.txt | head -1

echo ""
echo "Single Character Performance (SIMD enabled):"
grep "Testing pattern: a\* against 32-character string" /tmp/single_char_results.txt | head -1

echo ""
echo "🎯 Key Findings:"
echo "================"
echo ""

# Check if both are using SIMD
if grep -q "SIMD ENABLED" /tmp/char_class_results.txt; then
    echo "✅ Character class patterns are using SIMD"
else
    echo "❌ Character class patterns are NOT using SIMD"
fi

if grep -q "SIMD ENABLED" /tmp/single_char_results.txt; then
    echo "✅ Single character patterns are using SIMD"
else
    echo "❌ Single character patterns are NOT using SIMD"
fi

echo ""
echo "🧹 Cleaning up..."
rm -f /tmp/char_class_results.txt /tmp/single_char_results.txt

echo "✅ Analysis complete!"
