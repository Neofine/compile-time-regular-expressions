#include <ctre.hpp>
#include <ctre/decomposition.hpp>
#include <string>

// Standard match - no checks
bool match_standard(const std::string& input) {
    return ctre::match<"a+">(input);
}

// With if constexpr check (should compile away to same code)
bool match_with_check(const std::string& input) {
    using tmp = typename ctll::parser<ctre::pcre, ctll::fixed_string{"a+"}, ctre::pcre_actions>::template output<ctre::pcre_context<>>;
    using AST = decltype(ctll::front(typename tmp::output_type::stack_type()));
    using RE = AST;
    
    // This check should be compile-time constant = false, so branch eliminated
    constexpr bool has_literal = ctre::decomposition::has_prefilter_literal<RE>;
    
    if constexpr (has_literal) {
        // This code should NEVER be compiled in
        return false;
    }
    
    return ctre::match<"a+">(input);
}

int main() { return 0; }
