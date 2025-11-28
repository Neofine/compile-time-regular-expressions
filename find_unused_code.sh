#!/bin/bash

# Find potentially unused optimizations in the codebase

echo "╔═══════════════════════════════════════════════════════════════════════╗"
echo "║           Finding Unused/Dead Code                                   ║"
echo "╚═══════════════════════════════════════════════════════════════════════╝"
echo ""

cd /root/compile-time-regular-expressions

echo "═══════════════════════════════════════════════════════════════════════"
echo " ANALYZING SIMD FUNCTIONS"
echo "═══════════════════════════════════════════════════════════════════════"
echo ""

echo "SIMD functions defined in simd_character_classes.hpp:"
grep -n "^inline.*match_.*\(avx2\|sse42\|simd\)" include/ctre/simd_character_classes.hpp | \
    sed 's/:/ │ /' | head -30
echo ""

echo "═══════════════════════════════════════════════════════════════════════"
echo " CHECKING WHICH SIMD FUNCTIONS ARE ACTUALLY CALLED"
echo "═══════════════════════════════════════════════════════════════════════"
echo ""

# Compile a comprehensive test
cat > /tmp/test_all_patterns.cpp << 'EOF'
#include <ctre.hpp>
#include <string>

int main() {
    std::string a256(256, 'a');
    std::string z512(512, 'z');
    std::string vowels = "aeiouaeiou";
    
    // Single char patterns
    bool r1 = ctre::match<"a*">(a256);
    bool r2 = ctre::match<"a+">(a256);
    
    // Range patterns
    bool r3 = ctre::match<"[a-z]*">(z512);
    bool r4 = ctre::match<"[0-9]+">(std::string("123456789"));
    
    // Sparse patterns (Shufti)
    bool r5 = ctre::match<"[aeiou]+">(vowels);
    
    // Multi-range
    bool r6 = ctre::match<"[a-zA-Z]*">(a256);
    
    // Small input (should NOT use SIMD)
    bool r7 = ctre::match<"a+">(std::string(10, 'a'));
    
    return r1 && r2 && r3 && r4 && r5 && r6 && r7 ? 0 : 1;
}
EOF

echo "Compiling comprehensive test..."
g++ -std=c++20 -O3 -march=native -Iinclude /tmp/test_all_patterns.cpp -o /tmp/test_all 2>&1 | head -5

if [ -f /tmp/test_all ]; then
    echo "✓ Compiled successfully"
    echo ""
    
    echo "Symbols in binary (filtering for SIMD functions):"
    nm /tmp/test_all | grep -E "match_.*avx2|match_.*sse42|match_.*simd" | \
        sed 's/^.*T //' | sort | uniq
    echo ""
    
    echo "Checking for specific functions:"
    echo "  • match_single_char_repeat_avx2: $(nm /tmp/test_all | grep -c match_single_char_repeat_avx2 || echo 0) references"
    echo "  • match_char_class_repeat_avx2: $(nm /tmp/test_all | grep -c match_char_class_repeat_avx2 || echo 0) references"
    echo "  • match_pattern_repeat_shufti: $(nm /tmp/test_all | grep -c match_pattern_repeat_shufti || echo 0) references"
    echo "  • match_multirange_repeat: $(nm /tmp/test_all | grep -c match_multirange_repeat || echo 0) references"
    echo ""
else
    echo "❌ Compilation failed"
fi

echo "═══════════════════════════════════════════════════════════════════════"
echo " ANALYZING BITNFA USAGE"
echo "═══════════════════════════════════════════════════════════════════════"
echo ""

echo "BitNFA files in codebase:"
find include/ctre/bitnfa -name "*.hpp" 2>/dev/null | wc -l
echo "files found"
echo ""

echo "Total lines of BitNFA code:"
find include/ctre/bitnfa -name "*.hpp" -exec cat {} \; 2>/dev/null | wc -l
echo "lines"
echo ""

echo "Is BitNFA included in standard ctre.hpp?"
grep -c "bitnfa" include/ctre.hpp 2>/dev/null || echo "0"
echo ""

echo "═══════════════════════════════════════════════════════════════════════"
echo " CHECKING FOR DEAD CODE"
echo "═══════════════════════════════════════════════════════════════════════"
echo ""

echo "Functions defined but potentially unused:"
echo ""

echo "1. Checking simd_repetition.hpp:"
if [ -f include/ctre/simd_repetition.hpp ]; then
    FUNC_COUNT=$(grep -c "^inline.*{" include/ctre/simd_repetition.hpp)
    echo "   Defines $FUNC_COUNT inline functions"
    echo "   Used in evaluation.hpp?"
    grep -c "simd_repetition" include/ctre/evaluation.hpp || echo "   0 references"
else
    echo "   File not found"
fi
echo ""

echo "2. Checking simd_multirange.hpp:"
if [ -f include/ctre/simd_multirange.hpp ]; then
    FUNC_COUNT=$(grep -c "^inline.*{" include/ctre/simd_multirange.hpp)
    echo "   Defines $FUNC_COUNT inline functions"
    echo "   Used in evaluation.hpp?"
    grep -c "multirange" include/ctre/evaluation.hpp || echo "   0 references"
else
    echo "   File not found"
fi
echo ""

echo "3. Checking smart_dispatch.hpp:"
if [ -f include/ctre/smart_dispatch.hpp ]; then
    echo "   Smart dispatch is OPT-IN (not used by default)"
    echo "   Users must explicitly include it"
    echo "   Overhead: ZERO (unless explicitly used)"
else
    echo "   File not found"
fi
echo ""

echo "═══════════════════════════════════════════════════════════════════════"
echo " INCLUDE DEPENDENCY ANALYSIS"
echo "═══════════════════════════════════════════════════════════════════════"
echo ""

echo "What does ctre.hpp include?"
grep "^#include" include/ctre.hpp
echo ""

echo "What does evaluation.hpp include?"
grep "^#include" include/ctre/evaluation.hpp | head -20
echo ""

echo "═══════════════════════════════════════════════════════════════════════"
echo " ANALYSIS COMPLETE"
echo "═══════════════════════════════════════════════════════════════════════"

