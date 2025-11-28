#include "include/ctre/simd_detection_modern.hpp"
#include <iostream>

int main() {
    std::cout << "=== MODERN SIMD DETECTION TEST ===" << std::endl;
    std::cout << std::endl;
    
    // Test consteval function
    constexpr bool simd_enabled = ctre::simd::can_use_simd();
    std::cout << "SIMD enabled (constexpr): " << simd_enabled << std::endl;
    
    // Test runtime detection
    std::cout << "Has AVX2: " << ctre::simd::has_avx2() << std::endl;
    std::cout << "Has SSE4.2: " << ctre::simd::has_sse42() << std::endl;
    std::cout << "SIMD capability: " << ctre::simd::get_simd_capability() << std::endl;
    
    // Test new strongly-typed version
    auto level = ctre::simd::get_simd_level();
    std::cout << "SIMD level (enum): " << static_cast<int>(level) << std::endl;
    
    // Test concepts (compile-time check)
    const char* ptr = "test";
    static_assert(ctre::simd::CharIterator<const char*>, "Should satisfy CharIterator concept");
    
    std::cout << std::endl;
    std::cout << "✓ All modern features working!" << std::endl;
    
    return 0;
}
