#pragma once
/**
 * Pattern Registry
 * 
 * Centralized definition of benchmark patterns organized by category.
 * To add a new pattern:
 *   1. Add a generator function in patterns.hpp
 *   2. Add an entry in the appropriate category below
 *   3. The benchmark will automatically pick it up
 */

#include "patterns.hpp"
#include "benchmark_config.hpp"
#include <string>
#include <vector>
#include <functional>

namespace bench::registry {

// ============================================================================
// PATTERN DEFINITION
// ============================================================================

struct Pattern {
    std::string name;              // Display name (e.g., "digits")
    std::string ctre_pattern;      // Compile-time pattern for CTRE
    std::string runtime_pattern;   // Runtime pattern for other engines
    InputGenerator generator;       // Input generator function
    std::string description;       // Optional description
};

// ============================================================================
// CATEGORY DEFINITION
// ============================================================================

struct Category {
    std::string name;
    std::vector<Pattern> patterns;
    std::vector<size_t> sizes;
    int input_count;
    bool skip_std_regex;  // Skip std::regex for this category
};

// ============================================================================
// PATTERN CATEGORIES
// ============================================================================

inline Category simple_patterns() {
    return {
        "Simple",
        {
            {"digits", "[0-9]+", "[0-9]+", gen_digits, "Digit repetition"},
            {"lowercase", "[a-z]+", "[a-z]+", gen_letters, "Lowercase letters"},
            {"uppercase", "[A-Z]+", "[A-Z]+", gen_upper, "Uppercase letters"},
            {"vowels", "[aeiou]+", "[aeiou]+", gen_vowels, "Sparse character set"},
            {"alphanumeric", "[a-zA-Z0-9]+", "[a-zA-Z0-9]+", gen_alnum, "Alphanumeric"},
        },
        config::sizes_standard(),
        config::INPUTS_DEFAULT,
        false
    };
}

inline Category complex_patterns() {
    return {
        "Complex",
        {
            {"decimal", "[0-9]+\\.[0-9]+", "[0-9]+\\.[0-9]+", gen_decimal, "Decimal numbers"},
            {"hex", "[0-9a-fA-F]+", "[0-9a-fA-F]+", gen_hex, "Hexadecimal"},
            {"identifier", "[a-zA-Z_][a-zA-Z0-9_]*", "[a-zA-Z_][a-zA-Z0-9_]*", gen_json_key, "Identifiers"},
            {"url", "http://[a-z]+", "http://[a-z]+", gen_url, "Simple URLs"},
            {"key_value", "[a-z]+=[0-9]+", "[a-z]+=[0-9]+", gen_key_value, "Key=value pairs"},
            {"http_method", "(GET|POST)/[a-z]+", "(GET|POST)/[a-z]+", gen_http_method, "HTTP methods"},
            {"letters_digits", "[a-z]+[0-9]+", "[a-z]+[0-9]+", gen_letters_then_digits, "Letters then digits"},
            {"http_header", "[A-Za-z\\-]+: [a-zA-Z0-9 ]+", "[A-Za-z\\-]+: [a-zA-Z0-9 ]+", gen_http_header_full, "HTTP headers"},
            {"log_time", "[0-9]+:[0-9]+:[0-9]+", "[0-9]+:[0-9]+:[0-9]+", gen_log_time_full, "Log timestamps"},
        },
        config::sizes_standard(),
        config::INPUTS_DEFAULT,
        false
    };
}

inline Category scaling_patterns() {
    return {
        "Scaling",
        {
            {"alt_2", "(a|b)+", "(a|b)+", gen_ab, "2-way alternation"},
            {"alt_4", "(a|b|c|d)+", "(a|b|c|d)+", gen_abcd, "4-way alternation"},
            {"class_2", "[ab]+", "[ab]+", gen_ab, "2-char class"},
            {"class_4", "[abcd]+", "[abcd]+", gen_abcd, "4-char class"},
            {"class_26", "[a-z]+", "[a-z]+", gen_letters, "26-char class"},
        },
        config::sizes_standard(),
        config::INPUTS_DEFAULT,
        false
    };
}

inline Category realworld_patterns() {
    return {
        "RealWorld",
        {
            {"ipv4", "[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+", "[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+", gen_ipv4_full, "IPv4 addresses"},
            {"uuid", "[0-9a-f]+-[0-9a-f]+-[0-9a-f]+-[0-9a-f]+-[0-9a-f]+", "[0-9a-f]+-[0-9a-f]+-[0-9a-f]+-[0-9a-f]+-[0-9a-f]+", gen_uuid_full, "UUIDs"},
            {"email", "[a-z]+@[a-z]+\\.[a-z]+", "[a-z]+@[a-z]+\\.[a-z]+", gen_email_full, "Email addresses"},
            {"date", "[0-9]+-[0-9]+-[0-9]+", "[0-9]+-[0-9]+-[0-9]+", gen_date_full, "Dates"},
        },
        config::sizes_standard(),
        config::INPUTS_DEFAULT,
        false
    };
}

inline Category nonmatch_patterns() {
    return {
        "NonMatch",
        {
            // Basic non-matches
            {"digits_on_letters", "[0-9]+", "[0-9]+", gen_pure_letters, "Digit pattern on letter input"},
            {"letters_on_digits", "[a-z]+", "[a-z]+", gen_pure_digits, "Letter pattern on digit input"},
            {"url_on_digits", "http://[a-z]+", "http://[a-z]+", gen_pure_digits, "URL on digit input"},
            
            // Dominator prefilter patterns
            {"dom_suffix", "[a-z]+test", "[a-z]+test", gen_no_test_literal, "Suffix dominator"},
            {"dom_prefix", "test[a-z]+", "test[a-z]+", gen_no_test_literal, "Prefix dominator"},
            {"dom_middle", "[a-z]+test[0-9]+", "[a-z]+test[0-9]+", gen_no_test_literal, "Middle dominator"},
            {"dom_alt", "(foo|bar)test", "(foo|bar)test", gen_no_test_literal, "Alternation dominator"},
            
            // Region prefilter patterns
            {"region_suffix", "(runn|jump|walk)ing", "(runn|jump|walk)ing", gen_no_ing_suffix, "Common suffix"},
            
            // URL with literal prefilter
            {"dom_url", "http://[a-z]+\\.[a-z]+", "http://[a-z]+\\.[a-z]+", gen_no_http_literal, "URL literal prefilter"},
        },
        config::sizes_standard(),
        config::INPUTS_DEFAULT,
        false
    };
}

inline Category small_patterns() {
    return {
        "Small",
        {
            {"digits", "[0-9]+", "[0-9]+", gen_digits, "Small input digits"},
        },
        config::sizes_small(),
        config::INPUTS_DEFAULT,
        false
    };
}

inline Category large_patterns() {
    return {
        "Large",
        {
            {"digits", "[0-9]+", "[0-9]+", gen_digits, "Large input digits"},
        },
        config::sizes_large(),
        config::INPUTS_LARGE,
        true  // Skip std::regex
    };
}

inline Category fallback_patterns() {
    return {
        "Fallback",
        {
            // SIMD-optimizable (single-char backrefs)
            {"backref_repeat", "(.)\\1+", "(.)\\1+", gen_repeated_char, "Single-char backref"},
            {"nested_backref", "((.)\\2)+", "((.)\\2)+", gen_repeated_char, "Nested backref"},
            
            // Truly non-SIMD patterns
            {"lazy_star", "[a-z]*?x", "[a-z]*?x", gen_lazy_match, "Lazy star"},
            {"lazy_plus", "[a-z]+?x", "[a-z]+?x", gen_lazy_match, "Lazy plus"},
            {"lookahead_pos", "[a-z](?=[0-9])", "[a-z](?=[0-9])", gen_lookahead, "Positive lookahead"},
            {"lookahead_neg", "[a-z](?![0-9])", "[a-z](?![0-9])", gen_letters, "Negative lookahead"},
            {"group_repeat", "(abc)+", "(abc)+", gen_repeated_group, "Group repetition"},
        },
        config::sizes_standard(),
        config::INPUTS_DEFAULT,
        false
    };
}

// ============================================================================
// GET ALL CATEGORIES
// ============================================================================

inline std::vector<Category> all_categories() {
    return {
        simple_patterns(),
        complex_patterns(),
        scaling_patterns(),
        realworld_patterns(),
        nonmatch_patterns(),
        small_patterns(),
        large_patterns(),
        fallback_patterns(),
    };
}

// Get category by name (case-insensitive)
inline const Category* get_category(const std::string& name) {
    static auto categories = all_categories();
    for (const auto& cat : categories) {
        std::string cat_lower = cat.name;
        std::string name_lower = name;
        for (auto& c : cat_lower) c = std::tolower(c);
        for (auto& c : name_lower) c = std::tolower(c);
        if (cat_lower == name_lower) {
            return &cat;
        }
    }
    return nullptr;
}

} // namespace bench::registry


