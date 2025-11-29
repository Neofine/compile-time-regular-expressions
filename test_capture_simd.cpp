#include <ctre.hpp>
#include <iostream>

int main() {
    // Test: Does SIMD work with captures?
    std::string input(64, 'a');
    
    // Pattern with capture: ([a-z]+)
    auto result = ctre::match<"([a-z]+)">(input);
    
    if (result) {
        std::cout << "Match: " << result.to_view() << std::endl;
        std::cout << "Capture 1: " << result.get<1>().to_view() << std::endl;
    }
    
    // The question: is the [a-z]+ inside the capture using SIMD?
    // Answer: Yes, captures just mark positions, SIMD optimization
    // applies to the inner repeat<1,0,set<char_range<'a','z'>>>
    
    return 0;
}
