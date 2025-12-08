#!/bin/bash

cd "$(dirname "$0")"

CXX="${CXX:-g++}"
CXXFLAGS="-std=c++20 -O2 -Wall -Wextra -I../include"

echo "=== CTRE Test Suite ==="
echo "Compiler: $CXX"
echo ""

PASSED=0
FAILED=0

mkdir -p build

for src in *.cpp; do
    test="${src%.cpp}"
    printf "%-45s" "${test}..."
    
    if $CXX $CXXFLAGS "$src" -o "build/${test}" 2>/dev/null; then
        if "./build/${test}" >/dev/null 2>/dev/null; then
            echo "PASSED"
            PASSED=$((PASSED + 1))
        else
            echo "FAILED (runtime)"
            FAILED=$((FAILED + 1))
        fi
    else
        echo "FAILED (compile)"
        FAILED=$((FAILED + 1))
    fi
done

echo ""
echo "=== Results: ${PASSED} passed, ${FAILED} failed ==="

if [ $FAILED -gt 0 ]; then
    exit 1
fi
exit 0
