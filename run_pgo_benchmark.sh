#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "=========================================="
echo "Profile-Guided Optimization (PGO) Setup"
echo "=========================================="
echo ""

# Clean previous profile data
rm -rf /tmp/pgo_profile
mkdir -p /tmp/pgo_profile

echo "Step 1: Compiling with profile generation..."
g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 -msse4.2 \
    -fprofile-generate=/tmp/pgo_profile \
    tests/master_benchmark.cpp -o /tmp/ctre_profile 2>&1 | head -20

if [ $? -ne 0 ]; then
    echo "ERROR: Profile generation build failed"
    exit 1
fi

echo "✓ Profile build complete"
echo ""

echo "Step 2: Running benchmark to collect profile data..."
echo "(This will be slow due to instrumentation overhead)"
echo ""

# Run multiple times to get good profile data
for i in {1..3}; do
    echo "Profile run $i/3..."
    /tmp/ctre_profile > /dev/null 2>&1
done

echo ""
echo "✓ Profile data collected in /tmp/pgo_profile"
echo ""

# Check profile data was generated
PROFILE_FILES=$(find /tmp/pgo_profile -name "*.gcda" | wc -l)
echo "Profile files generated: $PROFILE_FILES"

if [ $PROFILE_FILES -eq 0 ]; then
    echo "ERROR: No profile data collected!"
    exit 1
fi

echo ""
echo "Step 3: Compiling with profile-guided optimization..."
g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 -msse4.2 \
    -fprofile-use=/tmp/pgo_profile -fprofile-correction \
    tests/master_benchmark.cpp -o /tmp/ctre_pgo 2>&1 | head -20

if [ $? -ne 0 ]; then
    echo "ERROR: PGO build failed"
    exit 1
fi

echo "✓ PGO build complete"
echo ""

# Compare binary sizes
SIZE_NORMAL=$(stat -c%s /tmp/ctre_final 2>/dev/null || echo "N/A")
SIZE_PGO=$(stat -c%s /tmp/ctre_pgo)

echo "=========================================="
echo "Binary Size Comparison"
echo "=========================================="
echo "Normal build: $SIZE_NORMAL bytes"
echo "PGO build:    $SIZE_PGO bytes"

if [ "$SIZE_NORMAL" != "N/A" ]; then
    REDUCTION=$(echo "scale=1; (1 - $SIZE_PGO/$SIZE_NORMAL) * 100" | bc -l 2>/dev/null || echo "?")
    echo "Reduction:    ${REDUCTION}%"
fi

echo ""
echo "Step 4: Running PGO benchmark..."
echo ""

/tmp/ctre_pgo

echo ""
echo "=========================================="
echo "PGO Optimization Complete!"
echo "=========================================="
