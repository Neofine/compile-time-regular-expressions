#include <ctre.hpp>
#include <iostream>
#include <string>
#include <type_traits>

// Compile-time pattern analysis tool
// Shows which matching strategy CTRE uses for each pattern

template<typename Pattern>
struct PatternAnalyzer {
    static constexpr bool is_repetition = false;
    static constexpr bool is_alternation = false;
    static constexpr bool is_single_char = false;
    static constexpr bool is_char_class = false;
    static constexpr bool is_literal = false;
    static constexpr bool can_use_simd = false;
    static constexpr const char* strategy = "Scalar/Glushkov NFA";
};

// Helper to analyze ctre patterns
template<auto Pattern>
void analyze_pattern(const char* name, const char* pattern_str) {
    using namespace ctre;
    
    std::cout << "Pattern: " << name << " (" << pattern_str << ")" << std::endl;
    
    // Parse pattern
    using parsed = typename ctll::parser<pcre, decltype(Pattern), pcre_actions>::template output<pcre_context<>>;
    static_assert(parsed(), "Pattern has syntax error");
    
    using AST = decltype(ctll::front(typename parsed::output_type::stack_type()));
    
    // Detect pattern type
    std::cout << "  Strategy: ";
    
    // Check if it's a repetition (a*, a+, [a-z]+, etc.)
    if constexpr (glushkov::is_repeat<AST>::value) {
        std::cout << "SIMD Character Repetition";
        
        // Try to determine sub-type
        using Content = typename AST::content;
        std::cout << " (";
        
        // Check what's being repeated
        if constexpr (requires { Content::min_char; Content::max_char; }) {
            char min = Content::min_char;
            char max = Content::max_char;
            int range = max - min + 1;
            
            if (range == 1) {
                std::cout << "Single char";
            } else if (range <= 26) {
                std::cout << "Small range [" << min << "-" << max << "]";
            } else {
                std::cout << "Large range [" << min << "-" << max << "]";
            }
        } else {
            std::cout << "Complex char class";
        }
        std::cout << ")";
        
    } else if constexpr (glushkov::is_select<AST>::value) {
        std::cout << "Alternation (Glushkov NFA with backtracking)";
    } else if constexpr (glushkov::is_string<AST>::value) {
        std::cout << "Literal string";
    } else if constexpr (glushkov::is_character<AST>::value) {
        std::cout << "Single character";
    } else {
        std::cout << "Complex pattern (Glushkov NFA)";
    }
    
    std::cout << std::endl;
    std::cout << std::endl;
}

int main() {
    std::cout << "╔══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║     CTRE Pattern Strategy Analyzer                      ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;
    
    std::cout << "This tool shows which matching strategy CTRE uses for each pattern." << std::endl;
    std::cout << "Strategies:" << std::endl;
    std::cout << "  1. SIMD Character Repetition - Fastest (AVX2/SSE4.2)" << std::endl;
    std::cout << "  2. Literal String - Fast (memcmp/SIMD)" << std::endl;
    std::cout << "  3. Single Character - Fast (scalar)" << std::endl;
    std::cout << "  4. Alternation - Slower (Glushkov NFA with backtracking)" << std::endl;
    std::cout << "  5. Complex Pattern - Slowest (Full Glushkov NFA)" << std::endl;
    std::cout << std::endl;
    std::cout << "═══════════════════════════════════════════════════════════" << std::endl;
    std::cout << std::endl;
    
    // Analyze benchmark patterns
    analyze_pattern<"a*">("a*_16", "a*");
    analyze_pattern<"a+">("a+_16", "a+");
    analyze_pattern<"a+">("a+_32", "a+");
    analyze_pattern<"[a-z]*">("[a-z]*_512", "[a-z]*");
    analyze_pattern<"[0-9]+">("[0-9]+_256", "[0-9]+");
    analyze_pattern<"[aeiou]+">("vowels", "[aeiou]+");
    analyze_pattern<"Huck[a-zA-Z]+|Saw[a-zA-Z]+">("complex_alt", "Huck[a-zA-Z]+|Saw[a-zA-Z]+");
    analyze_pattern<"[a-q][^u-z]{13}x">("negated_class", "[a-q][^u-z]{13}x");
    analyze_pattern<"[a-zA-Z]+ing">("suffix_ing", "[a-zA-Z]+ing");
    analyze_pattern<"\\s+ing">("whitespace_ing", "\\s+ing");
    
    return 0;
}

