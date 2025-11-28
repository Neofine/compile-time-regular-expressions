#!/bin/bash

# Analyze binary size and overhead of different optimizations

echo "╔═══════════════════════════════════════════════════════════════════════╗"
echo "║           Binary Size & Overhead Analysis                            ║"
echo "╚═══════════════════════════════════════════════════════════════════════╝"
echo ""

cd /root/compile-time-regular-expressions

# Create test files with different optimization levels
cat > /tmp/test_minimal.cpp << 'EOF'
#include <ctre.hpp>
#include <string>

int main() {
    // Just one simple pattern
    std::string input = "aaaaaaaaaaaaaaaa";
    return ctre::match<"a+">(input) ? 0 : 1;
}
EOF

cat > /tmp/test_with_simd.cpp << 'EOF'
#include <ctre.hpp>
#include <string>

int main() {
    // Pattern that uses SIMD
    std::string input(256, 'a');
    return ctre::match<"a*">(input) ? 0 : 1;
}
EOF

cat > /tmp/test_with_bitnfa.cpp << 'EOF'
#include <ctre.hpp>
#include <ctre/bitnfa/integration.hpp>
#include <string>

int main() {
    // Pattern that uses BitNFA
    std::string input = "Huckleberry";
    return ctre::bitnfa::match<"Tom|Sawyer|Huckleberry|Finn">(input).matched ? 0 : 1;
}
EOF

cat > /tmp/test_everything.cpp << 'EOF'
#include <ctre.hpp>
#include <ctre/bitnfa/integration.hpp>
#include <ctre/smart_dispatch.hpp>
#include <string>

int main() {
    // Multiple patterns
    std::string input1(256, 'a');
    std::string input2 = "Huckleberry";
    
    bool r1 = ctre::match<"a*">(input1);
    bool r2 = ctre::match<"[a-z]*">(input1);
    bool r3 = ctre::bitnfa::match<"Tom|Sawyer|Huckleberry|Finn">(input2).matched;
    
    return (r1 && r2 && r3) ? 0 : 1;
}
EOF

echo "═══════════════════════════════════════════════════════════════════════"
echo " COMPILING TEST BINARIES"
echo "═══════════════════════════════════════════════════════════════════════"
echo ""

echo "1. Minimal (single pattern, no SIMD)..."
g++ -std=c++20 -O3 -Iinclude /tmp/test_minimal.cpp -o /tmp/test_minimal 2>&1 | head -5
MINIMAL_SIZE=$(stat -c%s /tmp/test_minimal 2>/dev/null || echo "0")
echo "   Size: $MINIMAL_SIZE bytes"
echo ""

echo "2. With SIMD (256-byte pattern)..."
g++ -std=c++20 -O3 -march=native -Iinclude /tmp/test_with_simd.cpp -o /tmp/test_with_simd 2>&1 | head -5
SIMD_SIZE=$(stat -c%s /tmp/test_with_simd 2>/dev/null || echo "0")
echo "   Size: $SIMD_SIZE bytes"
echo ""

echo "3. With BitNFA..."
g++ -std=c++20 -O3 -march=native -Iinclude /tmp/test_with_bitnfa.cpp -o /tmp/test_with_bitnfa 2>&1 | head -5
BITNFA_SIZE=$(stat -c%s /tmp/test_with_bitnfa 2>/dev/null || echo "0")
echo "   Size: $BITNFA_SIZE bytes"
echo ""

echo "4. Everything (SIMD + BitNFA + Smart Dispatch)..."
g++ -std=c++20 -O3 -march=native -Iinclude /tmp/test_everything.cpp -o /tmp/test_everything 2>&1 | head -5
EVERYTHING_SIZE=$(stat -c%s /tmp/test_everything 2>/dev/null || echo "0")
echo "   Size: $EVERYTHING_SIZE bytes"
echo ""

echo "═══════════════════════════════════════════════════════════════════════"
echo " SIZE COMPARISON"
echo "═══════════════════════════════════════════════════════════════════════"
echo ""

if [ "$MINIMAL_SIZE" -gt 0 ]; then
    SIMD_OVERHEAD=$((SIMD_SIZE - MINIMAL_SIZE))
    BITNFA_OVERHEAD=$((BITNFA_SIZE - MINIMAL_SIZE))
    EVERYTHING_OVERHEAD=$((EVERYTHING_SIZE - MINIMAL_SIZE))
    
    echo "Minimal (baseline):     $MINIMAL_SIZE bytes"
    echo "With SIMD:              $SIMD_SIZE bytes (+$SIMD_OVERHEAD bytes, +$(echo "scale=1; $SIMD_OVERHEAD * 100 / $MINIMAL_SIZE" | bc)%)"
    echo "With BitNFA:            $BITNFA_SIZE bytes (+$BITNFA_OVERHEAD bytes, +$(echo "scale=1; $BITNFA_OVERHEAD * 100 / $MINIMAL_SIZE" | bc)%)"
    echo "Everything:             $EVERYTHING_SIZE bytes (+$EVERYTHING_OVERHEAD bytes, +$(echo "scale=1; $EVERYTHING_OVERHEAD * 100 / $MINIMAL_SIZE" | bc)%)"
    echo ""
fi

echo "═══════════════════════════════════════════════════════════════════════"
echo " SYMBOL ANALYSIS (what's actually compiled in)"
echo "═══════════════════════════════════════════════════════════════════════"
echo ""

if [ -f /tmp/test_everything ]; then
    echo "Looking for SIMD functions in 'Everything' binary:"
    nm /tmp/test_everything | grep -i "simd\|avx\|sse" | head -20
    echo ""
    
    echo "Looking for BitNFA functions:"
    nm /tmp/test_everything | grep -i "bitnfa\|nfa" | head -20
    echo ""
    
    echo "Total symbols:"
    nm /tmp/test_everything | wc -l
    echo ""
fi

echo "═══════════════════════════════════════════════════════════════════════"
echo " ANALYSIS COMPLETE"
echo "═══════════════════════════════════════════════════════════════════════"

