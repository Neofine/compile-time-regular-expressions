#!/bin/bash
echo "=== INVESTIGATING a+ vs a* PERFORMANCE ===" 
echo ""
echo "a*_16: 10.78x (excellent!)"
echo "a+_16:  6.46x (why so low?)"
echo ""

# Create test programs
cat << 'CPP' > /tmp/test_a_star.cpp
#include <ctre.hpp>
#include <string_view>

extern "C" __attribute__((noinline))
const char* bench_a_star(const char* b, const char* e) {
    return ctre::match<"a*">(std::string_view(b, e-b)).begin();
}

int main() {
    char d[16];
    for(int i=0;i<16;i++) d[i]='a';
    return bench_a_star(d, d+16) != nullptr;
}
CPP

cat << 'CPP' > /tmp/test_a_plus.cpp
#include <ctre.hpp>
#include <string_view>

extern "C" __attribute__((noinline))
const char* bench_a_plus(const char* b, const char* e) {
    return ctre::match<"a+">(std::string_view(b, e-b)).begin();
}

int main() {
    char d[16];
    for(int i=0;i<16;i++) d[i]='a';
    return bench_a_plus(d, d+16) != nullptr;
}
CPP

g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 /tmp/test_a_star.cpp -o /tmp/test_a_star 2>/dev/null
g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 /tmp/test_a_plus.cpp -o /tmp/test_a_plus 2>/dev/null

echo "Comparing assembly..."
echo ""

echo "a* (MinCount=0) branch count:"
objdump -d /tmp/test_a_star | awk '/bench_a_star>:/,/ret/' | grep -c "j[a-z]" | xargs -I {} echo "  {}"

echo "a+ (MinCount=1) branch count:"
objdump -d /tmp/test_a_plus | awk '/bench_a_plus>:/,/ret/' | grep -c "j[a-z]" | xargs -I {} echo "  {}"

echo ""
echo "Looking for MinCount checking code..."

# Check if there's extra MinCount validation
echo ""
echo "a* function size:"
objdump -d /tmp/test_a_star | awk '/bench_a_star>:/,/^$/' | wc -l | xargs -I {} echo "  {} lines"

echo "a+ function size:"
objdump -d /tmp/test_a_plus | awk '/bench_a_plus>:/,/^$/' | wc -l | xargs -I {} echo "  {} lines"

