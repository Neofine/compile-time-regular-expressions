#include <iostream>
#include "ctre/simd_character_classes.hpp"

int main() {
    std::cout << "Testing simd_pattern_trait for character<'a'>" << std::endl;
    
    // Test if the trait works
    using char_a = ctre::character<'a'>;
    
    std::cout << "simd_pattern_trait<char_a>::is_simd_optimizable: " 
              << ctre::simd::simd_pattern_trait<char_a>::is_simd_optimizable << std::endl;
    
    std::cout << "simd_pattern_trait<char_a>::single_char: " 
              << ctre::simd::simd_pattern_trait<char_a>::single_char << std::endl;
    
    // Test if the requires clause works
    if constexpr (requires { typename ctre::simd::simd_pattern_trait<char_a>::single_char; }) {
        std::cout << "requires clause PASSED for character<'a'>" << std::endl;
    } else {
        std::cout << "requires clause FAILED for character<'a'>" << std::endl;
    }
    
    // Test the is_char_range_set_trait
    std::cout << "is_char_range_set_trait<char_a>::value: " 
              << ctre::simd::is_char_range_set_trait<char_a>::value << std::endl;
    
    if constexpr (requires { typename ctre::simd::is_char_range_set_trait<char_a>::type; }) {
        std::cout << "is_char_range_set_trait requires clause PASSED" << std::endl;
    } else {
        std::cout << "is_char_range_set_trait requires clause FAILED" << std::endl;
    }
    
    return 0;
}
