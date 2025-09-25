#!/bin/bash

# Simple Character Class Benchmark
# ===============================

set -e

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
NC='\033[0m'

echo -e "${PURPLE}🚀 Simple Character Class Benchmark${NC}"
echo -e "${PURPLE}===================================${NC}"
echo

# Cleanup function
cleanup() {
    rm -f tests/simd_character_class_benchmark.o tests/simd_character_class_benchmark
    rm -f tests/simd_character_class_benchmark_disabled.o tests/simd_character_class_benchmark_disabled
    rm -f tests/simd_character_class_benchmark.d tests/simd_character_class_benchmark_disabled.d
}

trap cleanup EXIT INT TERM

# Build both versions
echo -e "${BLUE}Building benchmarks...${NC}"
make tests/simd_character_class_benchmark > /dev/null 2>&1
make tests/simd_character_class_benchmark_disabled > /dev/null 2>&1
echo -e "${GREEN}✅ Build complete${NC}"
echo

# Run SIMD version
echo -e "${BLUE}Running SIMD-enabled benchmark:${NC}"
echo -e "${GREEN}=== SIMD ENABLED ===${NC}"
./tests/simd_character_class_benchmark
echo

# Run non-SIMD version  
echo -e "${BLUE}Running SIMD-disabled benchmark:${NC}"
echo -e "${RED}=== SIMD DISABLED ===${NC}"
./tests/simd_character_class_benchmark_disabled
echo

echo -e "${PURPLE}📊 Quick Analysis:${NC}"
echo -e "${PURPLE}==================${NC}"
echo -e "${YELLOW}• Compare the 'Avg time' values above${NC}"
echo -e "${YELLOW}• Lower values = faster performance${NC}"
echo -e "${YELLOW}• SIMD should be faster for longer strings (40+ chars)${NC}"
echo -e "${YELLOW}• If SIMD is slower, the optimization may not be working${NC}"
echo

echo -e "${GREEN}🎉 Benchmark complete!${NC}"
