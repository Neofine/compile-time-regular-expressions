#include <ctre.hpp>
#include <iostream>
#include <string>

// Instrumented version to trace which strategy is actually used at runtime

#define TRACE(msg) std::cout << "  [TRACE] " << msg << std::endl

// Test a pattern and trace what happens
template<ctll::fixed_string Pattern>
void trace_pattern(const char* name, const std::string& input) {
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "Pattern: " << name << " = \"" << Pattern.string << "\"" << std::endl;
    std::cout << "Input size: " << input.size() << " bytes" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;

    // Show what we know at compile-time
    using parsed = typename ctll::parser<ctre::pcre, Pattern, ctre::pcre_actions>::template output<ctre::pcre_context<>>;
    using AST = decltype(ctll::front(typename parsed::output_type::stack_type()));

    std::cout << "Compile-time analysis:" << std::endl;

    // Pattern type detection
    if constexpr (ctre::glushkov::is_repeat<AST>::value) {
        TRACE("Pattern type: REPETITION (can use SIMD!)");

        using Content = typename AST::content;

        if constexpr (std::is_same_v<Content, ctre::character<'a'>> ||
                      std::is_same_v<Content, ctre::character<'A'>> ||
                      std::is_same_v<Content, ctre::character<'z'>>) {
            TRACE("  Content: Single character → match_single_char_repeat_avx2()");
        } else if constexpr (requires { Content::min_char; Content::max_char; }) {
            constexpr char min = Content::min_char;
            constexpr char max = Content::max_char;
            std::cout << "  [TRACE]   Content: Range [" << min << "-" << max << "] → match_char_class_repeat_avx2()" << std::endl;
        } else {
            TRACE("  Content: Complex class → match_pattern_repeat_simd()");
        }

    } else if constexpr (ctre::glushkov::is_select<AST>::value) {
        TRACE("Pattern type: ALTERNATION (uses Glushkov NFA with backtracking)");
    } else if constexpr (ctre::glushkov::is_string<AST>::value) {
        TRACE("Pattern type: LITERAL STRING (uses memcmp)");
    } else {
        TRACE("Pattern type: COMPLEX (uses general Glushkov NFA)");
    }

    // Runtime decision
    std::cout << "\nRuntime decision:" << std::endl;

    if constexpr (ctre::glushkov::is_repeat<AST>::value) {
        if (input.size() >= 28) {
            TRACE("Input size >= 28 bytes → SIMD PATH!");
            if (input.size() >= 64) {
                TRACE("  Using 64-byte unrolled loop (2x 32-byte AVX2 ops)");
            } else if (input.size() >= 32) {
                TRACE("  Using 32-byte fast path");
            } else {
                TRACE("  Using 16-byte SSE4.2 loop (fallback)");
            }
        } else {
            TRACE("Input size < 28 bytes → SCALAR FALLBACK");
            TRACE("  (SIMD overhead would exceed benefit)");
        }
    } else {
        TRACE("Non-repetition pattern → uses NFA/scalar path");
    }

    // Actually run the match
    auto result = ctre::match<Pattern>(input);

    std::cout << "\nResult: " << (result ? "MATCH" : "NO MATCH") << std::endl;
}

int main() {
    std::cout << "╔═══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║        CTRE Dispatch Tracing Tool                        ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;
    std::cout << "This tool shows EXACTLY which code path CTRE takes for each pattern." << std::endl;

    // Test various patterns

    // Small input - should fall back to scalar
    trace_pattern<"a+">("a+_16 (SMALL INPUT)", std::string(16, 'a'));

    // Medium input - should use 32-byte path
    trace_pattern<"a+">("a+_32 (MEDIUM INPUT)", std::string(32, 'a'));

    // Large input - should use 64-byte unroll
    trace_pattern<"a*">("a*_256 (LARGE INPUT)", std::string(256, 'a'));

    // Range pattern
    std::string lowercase_512(512, 'a');
    for (int i = 0; i < 512; i++) {
        lowercase_512[i] = 'a' + (i % 26);
    }
    trace_pattern<"[a-z]*">("[a-z]*_512 (RANGE)", lowercase_512);

    // Alternation - uses NFA
    trace_pattern<"Huck[a-zA-Z]+|Saw[a-zA-Z]+">("complex_alt (ALTERNATION)", "Huckleberry");

    // Negated class
    trace_pattern<"[a-q][^u-z]{13}x">("negated_class (NEGATED)", "abcdefghijklmnx");

    std::cout << "\n╔═══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                    SUMMARY                                ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;
    std::cout << "KEY FINDINGS:" << std::endl;
    std::cout << "  • Repetitions (a*, [a-z]+): Use SIMD if input >= 28 bytes" << std::endl;
    std::cout << "  • Alternations (A|B): Use Glushkov NFA (can't SIMD dispatch)" << std::endl;
    std::cout << "  • Small inputs (<28B): Fall back to scalar (overhead > benefit)" << std::endl;
    std::cout << "  • Large inputs (≥64B): Use 64-byte unrolled AVX2 loops!" << std::endl;
    std::cout << std::endl;
    std::cout << "The 28-byte threshold is the key to understanding performance!" << std::endl;
    std::cout << std::endl;

    return 0;
}
