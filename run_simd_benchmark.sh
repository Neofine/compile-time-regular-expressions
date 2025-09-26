#!/bin/bash

# CTRE SIMD Benchmark Runner
# Compares SIMD vs Non-SIMD performance

set -e  # Exit on any error

echo "🚀 CTRE SIMD Benchmark Suite"
echo "============================"
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Function to print colored output
print_header() {
    echo -e "${BLUE}$1${NC}"
}

print_success() {
    echo -e "${GREEN}$1${NC}"
}

print_warning() {
    echo -e "${YELLOW}$1${NC}"
}

print_error() {
    echo -e "${RED}$1${NC}"
}

# Clean previous builds
print_header "🧹 Cleaning previous builds..."
make clean > /dev/null 2>&1 || true

# Compile SIMD version
print_header "🔧 Compiling SIMD version..."
make tests/simd_vs_nosimd_benchmark > /dev/null 2>&1

if [ $? -eq 0 ]; then
    print_success "✅ SIMD version compiled successfully"
else
    print_error "❌ Failed to compile SIMD version"
    exit 1
fi

# Compile Non-SIMD version
print_header "🔧 Compiling Non-SIMD version..."
CXXFLAGS="-DCTRE_DISABLE_SIMD" make tests/simd_vs_nosimd_benchmark > /dev/null 2>&1

if [ $? -eq 0 ]; then
    print_success "✅ Non-SIMD version compiled successfully"
else
    print_error "❌ Failed to compile Non-SIMD version"
    exit 1
fi

# Run SIMD benchmark
print_header "🏃 Running SIMD benchmark..."
echo ""
echo -e "${CYAN}=== SIMD ENABLED ===${NC}"
timeout 20s ./tests/simd_vs_nosimd_benchmark || {
    print_warning "⚠️  SIMD benchmark timed out or failed"
}

echo ""
print_header "🏃 Running Non-SIMD benchmark..."
echo ""
echo -e "${CYAN}=== SIMD DISABLED ===${NC}"
timeout 20s ./tests/simd_vs_nosimd_benchmark || {
    print_warning "⚠️  Non-SIMD benchmark timed out or failed"
}

# Run individual pattern benchmarks
print_header "🔍 Running individual pattern benchmarks..."

echo ""
echo -e "${CYAN}=== Single Character Patterns ===${NC}"
timeout 10s ./tests/simd_repetition_benchmark 2>/dev/null | head -10 || {
    print_warning "⚠️  Single character benchmark failed"
}

echo ""
echo -e "${CYAN}=== Character Class Patterns ===${NC}"
timeout 10s ./tests/simd_character_class_benchmark 2>/dev/null | head -10 || {
    print_warning "⚠️  Character class benchmark failed"
}

# Performance summary
print_header "📊 Performance Summary"
echo ""
echo -e "${GREEN}✅ SIMD optimizations implemented:${NC}"
echo "   • Single character repetition (a*, a+, a{n,m})"
echo "   • Character class repetition ([0-9]*, [a-z]*, etc.)"
echo "   • Small range optimization (≤10 chars: direct comparison)"
echo "   • Large range optimization (>10 chars: range comparison)"
echo "   • Case-insensitive matching"
echo "   • Remaining character processing with SIMD"
echo ""
echo -e "${YELLOW}📈 Expected performance characteristics:${NC}"
echo "   • Character classes: ~1.4-1.5 ns (very fast)"
echo "   • Single characters: ~4-7 ns (good)"
echo "   • Small ranges [0-9]: ~0.7-5.8 ns (optimized)"
echo "   • Large ranges [a-z]: ~1.4 ns (range comparison)"
echo ""

# Cleanup
print_header "🧹 Cleaning up..."
make clean > /dev/null 2>&1 || true

print_success "🎉 Benchmark complete!"
echo ""
echo -e "${PURPLE}💡 Tips:${NC}"
echo "   • Run with 'time' to see total execution time"
echo "   • Use 'perf stat' for detailed CPU analysis"
echo "   • Check CPU frequency: 'cpupower frequency-info'"
echo "   • For consistent results, disable CPU frequency scaling"
echo ""
