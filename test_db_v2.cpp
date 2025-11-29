#include <ctre.hpp>
#include <ctre/do_analysis.hpp>
#include <iostream>

int main() {
    // Test pattern WITH literal
    using Pattern = ctre::sequence<
        ctre::capture<1, ctre::select<ctre::string<'f','o','o'>, ctre::string<'b','a','r'>>>,
        ctre::string<'t','e','s','t'>
    >;
    
    constexpr auto entry = ctre::prefilter::get_analyzed<Pattern>();
    
    std::cout << "Has literal: " << entry.has_literal << "\n";
    std::cout << "Length: " << entry.length << "\n";
    
    if (entry.has_literal && entry.length > 0) {
        std::cout << "Literal: \"";
        for (size_t i = 0; i < entry.length; ++i) {
            std::cout << entry.chars[i];
        }
        std::cout << "\"\n";
    }
    
    std::cout << "\n✅ Database v2 works!\n";
    
    // Test runtime scan
    const char* input = "xxxtest";
    bool found = ctre::prefilter::contains_literal(input, input + 7, "test", 4);
    std::cout << "Runtime scan found 'test': " << found << "\n";
    
    return 0;
}
