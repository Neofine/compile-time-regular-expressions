#!/bin/bash

# Master Test Runner - Runs all unit and integration tests

set -e

echo "==========================================================================="
echo "CTRE Comprehensive Test Suite"
echo "==========================================================================="
echo ""

CXXFLAGS="-std=c++20 -Iinclude -Isrell_include -O3 -march=native -mtune=native"
CXXFLAGS="$CXXFLAGS -mavx2 -msse4.2 -mfma -mbmi2 -mlzcnt -mpopcnt"
CXXFLAGS="$CXXFLAGS -funroll-loops -ffast-math -flto"

FAILED_TESTS=()
PASSED_TESTS=()

# Helper to run a test
run_test() {
    local test_name=$1
    local test_file="tests/${test_name}.cpp"
    local test_bin="tests/${test_name}"

    echo "==========================================================================="
    echo "Running: $test_name"
    echo "==========================================================================="

    # Compile
    if ! g++ $CXXFLAGS "$test_file" -o "$test_bin" -lstdc++ 2>&1; then
        echo "❌ COMPILATION FAILED: $test_name"
        FAILED_TESTS+=("$test_name (compilation)")
        return 1
    fi

    # Run
    if ! "./$test_bin" 2>&1; then
        echo "❌ TEST FAILED: $test_name"
        FAILED_TESTS+=("$test_name (runtime)")
        return 1
    fi

    echo "✓ PASSED: $test_name"
    echo ""
    PASSED_TESTS+=("$test_name")

    # Cleanup
    rm -f "$test_bin"
    return 0
}

# Unit Tests
echo "=== UNIT TESTS ==="
echo ""
run_test "unit_test_safeguard" || true
run_test "unit_test_type_unwrapper" || true
run_test "unit_test_literal_extraction" || true

echo ""
echo "=== INTEGRATION TESTS ==="
echo ""
run_test "integration_test_correctness" || true
run_test "integration_test_performance" || true

# Existing tests
echo ""
echo "=== EXISTING COMPONENT TESTS ==="
echo ""
run_test "test_glushkov" || true
run_test "test_dominators" || true
run_test "test_region_analysis" || true

echo ""
echo "==========================================================================="
echo "TEST SUMMARY"
echo "==========================================================================="
echo "Passed: ${#PASSED_TESTS[@]}"
echo "Failed: ${#FAILED_TESTS[@]}"
echo ""

if [ ${#FAILED_TESTS[@]} -gt 0 ]; then
    echo "Failed tests:"
    for test in "${FAILED_TESTS[@]}"; do
        echo "  ❌ $test"
    done
    exit 1
else
    echo "✓ ALL TESTS PASSED!"
    exit 0
fi
