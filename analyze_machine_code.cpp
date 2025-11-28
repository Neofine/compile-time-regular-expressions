#include <ctre.hpp>
#include <string>
#include <iostream>

// Force no inlining so we can see the exact code
extern "C" __attribute__((noinline)) 
const char* benchmark_a16(const char* begin, const char* end) {
    return ctre::match<"a+">(std::string_view(begin, end - begin)).begin();
}

extern "C" __attribute__((noinline))
const char* benchmark_range32(const char* begin, const char* end) {
    return ctre::match<"[a-z]+">(std::string_view(begin, end - begin)).begin();
}

int main() {
    std::string s16(16, 'a');
    std::string s32(32, 'a');
    
    auto r1 = benchmark_a16(s16.data(), s16.data() + s16.size());
    auto r2 = benchmark_range32(s32.data(), s32.data() + s32.size());
    
    return (r1 != nullptr) + (r2 != nullptr);
}
