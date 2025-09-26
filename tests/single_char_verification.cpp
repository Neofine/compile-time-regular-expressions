#include "ctre.hpp"
#include <iostream>
#include <string>

int main() {
    std::cout << "Single Character Pattern Verification Test\n";
    std::cout << "==========================================\n\n";
    
    int passed = 0;
    int failed = 0;
    
    // Test basic single character patterns
    auto test_pattern = [&](const std::string& pattern_name, auto pattern, const std::string& test_string, bool should_match) {
        try {
            bool actual_match = pattern.match(test_string).has_value();
            bool test_passed = (actual_match == should_match);
            
            if (test_passed) {
                std::cout << "✅ PASS: " << pattern_name << " against \"" << test_string << "\"\n";
                passed++;
            } else {
                std::cout << "❌ FAIL: " << pattern_name << " against \"" << test_string << "\""
                         << " (expected " << (should_match ? "match" : "no match")
                         << ", got " << (actual_match ? "match" : "no match") << ")\n";
                failed++;
            }
        } catch (const std::exception& e) {
            std::cout << "❌ ERROR: " << pattern_name << " against \"" << test_string << "\""
                     << " (exception: " << e.what() << ")\n";
            failed++;
        }
    };
    
    // Test a* patterns
    test_pattern("a*", "a*"_ctre, "aaaa", true);
    test_pattern("a*", "a*"_ctre, "bbbb", true);  // Should match (zero a's)
    test_pattern("a*", "a*"_ctre, "", true);      // Should match (zero a's)
    test_pattern("a*", "a*"_ctre, "aaabbb", true); // Should match (partial)
    
    // Test a+ patterns
    test_pattern("a+", "a+"_ctre, "aaaa", true);
    test_pattern("a+", "a+"_ctre, "bbbb", false); // Should not match
    test_pattern("a+", "a+"_ctre, "", false);     // Should not match
    test_pattern("a+", "a+"_ctre, "aaabbb", true); // Should match (partial)
    
    // Test a{3} patterns
    test_pattern("a{3}", "a{3}"_ctre, "aaa", true);
    test_pattern("a{3}", "a{3}"_ctre, "aa", false);
    test_pattern("a{3}", "a{3}"_ctre, "aaaa", true); // Should match (partial)
    
    // Test a{2,4} patterns
    test_pattern("a{2,4}", "a{2,4}"_ctre, "aa", true);
    test_pattern("a{2,4}", "a{2,4}"_ctre, "aaa", true);
    test_pattern("a{2,4}", "a{2,4}"_ctre, "aaaa", true);
    test_pattern("a{2,4}", "a{2,4}"_ctre, "aaaaa", true); // Should match (partial)
    test_pattern("a{2,4}", "a{2,4}"_ctre, "a", false);
    
    // Test different characters
    test_pattern("b*", "b*"_ctre, "bbbb", true);
    test_pattern("z+", "z+"_ctre, "zzzz", true);
    test_pattern("0*", "0*"_ctre, "0000", true);
    test_pattern("9+", "9+"_ctre, "9999", true);
    
    // Test mixed strings
    test_pattern("b*", "b*"_ctre, "aaabbb", true); // Should match (zero b's at start)
    test_pattern("b+", "b+"_ctre, "aaabbb", true); // Should match (partial)
    
    std::cout << "\n";
    std::cout << "Results: " << passed << " passed, " << failed << " failed\n";
    
    if (failed == 0) {
        std::cout << "🎉 All single character pattern tests passed!\n";
        return 0;
    } else {
        std::cout << "⚠️  Some tests failed. Please investigate.\n";
        return 1;
    }
}