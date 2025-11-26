// BitNFA Demo - Showcase the new string matching API
// Compile: g++ -std=c++20 -Iinclude -Isrell_include -O2 -march=native -mavx2 -msse4.2 examples/bitnfa_demo.cpp -o bitnfa_demo

#include <ctre.hpp>
#include <ctre/bitnfa/bitnfa_match.hpp>
#include <iostream>
#include <string_view>

using namespace ctre::bitnfa;

void demo_match() {
    std::cout << "=== match() - Full String Matching ===\n\n";

    // Example 1: Validate digit string
    std::cout << "Validating '12345' matches '[0-9]+':\n";
    if (match<"[0-9]+">("12345")) {
        std::cout << "  ✓ Valid!\n";
    }

    // Example 2: Email-like pattern (simplified)
    std::cout << "\nValidating 'user@domain' matches pattern:\n";
    if (match<"[a-z]+@[a-z]+">("user@domain")) {
        std::cout << "  ✓ Looks like an email!\n";
    }

    // Example 3: Must match entire string
    std::cout << "\nTrying to match 'abc' against 'abcd' (should fail):\n";
    if (!match<"abc">("abcd")) {
        std::cout << "  ✗ Correctly rejected (full match required)\n";
    }
}

void demo_search() {
    std::cout << "\n=== search() - Find First Occurrence ===\n\n";

    // Example 1: Find pattern in text
    std::string_view text = "The year is 2025, not 2024";
    auto result = search<"[0-9]+">(text);

    std::cout << "Searching for digits in: \"" << text << "\"\n";
    if (result) {
        auto match_text = result.to_view(text);
        std::cout << "  Found: \"" << match_text << "\" at position " << result.position << "\n";
    }

    // Example 2: Longest match semantics
    std::string_view code = "int value = 123456;";
    auto num = search<"[0-9]+">(code);

    std::cout << "\nSearching for number in: \"" << code << "\"\n";
    if (num) {
        std::cout << "  Found: \"" << num.to_view(code) << "\" (length=" << num.length << ")\n";
        std::cout << "  Note: Greedy match, takes all digits!\n";
    }
}

void demo_find_all() {
    std::cout << "\n=== find_all() - Extract All Matches ===\n\n";

    // Example 1: Extract all words
    std::string_view sentence = "The quick brown fox";
    auto words = find_all<"[a-z]+">(sentence);

    std::cout << "Extracting words from: \"" << sentence << "\"\n";
    std::cout << "  Found " << words.size() << " words:\n";
    for (const auto& match : words) {
        std::cout << "    - \"" << match.to_view(sentence) << "\"\n";
    }

    // Example 2: Extract all numbers
    std::string_view data = "Values: 10, 20, 30, 40";
    auto numbers = find_all<"[0-9]+">(data);

    std::cout << "\nExtracting numbers from: \"" << data << "\"\n";
    std::cout << "  Found " << numbers.size() << " numbers:\n";
    for (const auto& match : numbers) {
        std::cout << "    - " << match.to_view(data) << " at position " << match.position << "\n";
    }
}

void demo_patterns() {
    std::cout << "\n=== Pattern Showcase ===\n\n";

    std::cout << "Single char:    ";
    std::cout << (match<"a">("a") ? "✓" : "✗") << "\n";

    std::cout << "Sequence:       ";
    std::cout << (match<"abc">("abc") ? "✓" : "✗") << "\n";

    std::cout << "Alternation:    ";
    std::cout << (match<"a|b">("b") ? "✓" : "✗") << "\n";

    std::cout << "Star (0+):      ";
    std::cout << (match<"a*">("aaaa") ? "✓" : "✗") << "\n";

    std::cout << "Plus (1+):      ";
    std::cout << (match<"a+">("a") ? "✓" : "✗") << "\n";

    std::cout << "Combined:       ";
    std::cout << (match<"a+b*c">("aaabbbbc") ? "✓" : "✗") << "\n";
}

int main() {
    std::cout << "🚀 BitNFA String Matching API Demo\n";
    std::cout << "===================================\n\n";

    demo_match();
    demo_search();
    demo_find_all();
    demo_patterns();

    std::cout << "\n🎉 Demo complete!\n";
    std::cout << "All patterns compiled at compile-time, matched at runtime with SIMD!\n";

    return 0;
}
