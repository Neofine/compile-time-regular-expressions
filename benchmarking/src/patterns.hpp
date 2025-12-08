#pragma once
// Input generators for benchmarking

#include <functional>
#include <random>
#include <string>
#include <vector>

namespace bench {

using InputGenerator = std::function<std::vector<std::string>(size_t, int, unsigned int)>;

// Helper: generate strings from character set
inline std::vector<std::string> gen_from_chars(const char* chars, size_t nchars, size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<size_t> d(0, nchars - 1);
    for (int i = 0; i < count; i++) {
        std::string s(len, 'x');
        for (size_t j = 0; j < len; j++) s[j] = chars[d(rng)];
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// Helper: generate fixed literal
inline std::vector<std::string> gen_literal(const std::string& lit, int count) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    for (int i = 0; i < count; i++) inputs.push_back(lit);
    return inputs;
}

// Basic character classes
inline auto gen_digits(size_t len, int count, unsigned int seed) { return gen_from_chars("0123456789", 10, len, count, seed); }
inline auto gen_letters(size_t len, int count, unsigned int seed) { return gen_from_chars("abcdefghijklmnopqrstuvwxyz", 26, len, count, seed); }
inline auto gen_upper(size_t len, int count, unsigned int seed) { return gen_from_chars("ABCDEFGHIJKLMNOPQRSTUVWXYZ", 26, len, count, seed); }
inline auto gen_vowels(size_t len, int count, unsigned int seed) { return gen_from_chars("aeiou", 5, len, count, seed); }
inline auto gen_hex(size_t len, int count, unsigned int seed) { return gen_from_chars("0123456789abcdefABCDEF", 22, len, count, seed); }
inline auto gen_alnum(size_t len, int count, unsigned int seed) { return gen_from_chars("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789", 62, len, count, seed); }
inline auto gen_ab(size_t len, int count, unsigned int seed) { return gen_from_chars("ab", 2, len, count, seed); }
inline auto gen_abcd(size_t len, int count, unsigned int seed) { return gen_from_chars("abcd", 4, len, count, seed); }

// Aliases for non-match testing
inline auto gen_pure_letters(size_t len, int count, unsigned int seed) { return gen_letters(len, count, seed); }
inline auto gen_pure_digits(size_t len, int count, unsigned int seed) { return gen_digits(len, count, seed); }

// Character sets excluding specific letters (for prefilter testing)
inline auto gen_no_test_literal(size_t len, int count, unsigned int seed) { return gen_from_chars("abcdfghijklmnopqruvwxyz", 23, len, count, seed); }
inline auto gen_no_http_literal(size_t len, int count, unsigned int seed) { return gen_from_chars("abcdefgijklmnoqrsuvwxyz", 23, len, count, seed); }
inline auto gen_no_ing_suffix(size_t len, int count, unsigned int seed) { return gen_from_chars("abcdefhjklmopqrstuvwxyz", 23, len, count, seed); }

// Pattern-specific generators
inline std::vector<std::string> gen_decimal(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> d(0, 9);
    for (int i = 0; i < count; i++) {
        size_t dot_pos = len / 2;
        std::string s(len, '0');
        for (size_t j = 0; j < len; j++) s[j] = (j == dot_pos) ? '.' : ('0' + d(rng));
        inputs.push_back(std::move(s));
    }
    return inputs;
}

inline std::vector<std::string> gen_ipv4_full(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> octet(0, 255);
    for (int i = 0; i < count; i++) {
        std::string s = std::to_string(octet(rng)) + "." + std::to_string(octet(rng)) + "." +
                        std::to_string(octet(rng)) + "." + std::to_string(octet(rng));
        while (s.size() < len) s += std::to_string(octet(rng));
        s.resize(len > 7 ? len : 7);
        inputs.push_back(std::move(s));
    }
    return inputs;
}

inline std::vector<std::string> gen_uuid_full(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> h(0, 15);
    const char hex[] = "0123456789abcdef";
    for (int i = 0; i < count; i++) {
        std::string s;
        for (int j = 0; j < 8; j++) s += hex[h(rng)];
        s += '-';
        for (int seg = 0; seg < 4; seg++) {
            for (int j = 0; j < 4; j++) s += hex[h(rng)];
            if (seg < 3) s += '-';
        }
        while (s.size() < len) s += hex[h(rng)];
        inputs.push_back(s.substr(0, len > 36 ? len : 36));
    }
    return inputs;
}

inline std::vector<std::string> gen_email_full(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> l(0, 25);
    for (int i = 0; i < count; i++) {
        size_t part = (len > 6) ? (len - 6) / 3 : 1;
        std::string s;
        for (size_t j = 0; j < part; j++) s += 'a' + l(rng);
        s += '@';
        for (size_t j = 0; j < part; j++) s += 'a' + l(rng);
        s += '.';
        for (size_t j = 0; j < part; j++) s += 'a' + l(rng);
        inputs.push_back(std::move(s));
    }
    return inputs;
}

inline std::vector<std::string> gen_date_full(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> d(0, 9);
    for (int i = 0; i < count; i++) {
        std::string s;
        for (int j = 0; j < 4; j++) s += '0' + d(rng);
        s += '-';
        for (int j = 0; j < 2; j++) s += '0' + d(rng);
        s += '-';
        for (int j = 0; j < 2; j++) s += '0' + d(rng);
        while (s.size() < len) s += '0' + d(rng);
        inputs.push_back(s.substr(0, len > 10 ? len : 10));
    }
    return inputs;
}

inline std::vector<std::string> gen_url(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> l(0, 25);
    for (int i = 0; i < count; i++) {
        std::string s = "http://";
        while (s.size() < len) s += 'a' + l(rng);
        inputs.push_back(std::move(s));
    }
    return inputs;
}

inline std::vector<std::string> gen_key_value(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> l(0, 25), d(0, 9);
    for (int i = 0; i < count; i++) {
        size_t half = len / 2;
        std::string s;
        for (size_t j = 0; j < half; j++) s += 'a' + l(rng);
        s += '=';
        while (s.size() < len) s += '0' + d(rng);
        inputs.push_back(std::move(s));
    }
    return inputs;
}

inline std::vector<std::string> gen_http_method(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> m(0, 1), l(0, 25);
    for (int i = 0; i < count; i++) {
        std::string s = m(rng) ? "GET/" : "POST/";
        while (s.size() < len) s += 'a' + l(rng);
        inputs.push_back(std::move(s));
    }
    return inputs;
}

inline std::vector<std::string> gen_letters_then_digits(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> l(0, 25), d(0, 9);
    for (int i = 0; i < count; i++) {
        size_t half = len / 2;
        std::string s;
        for (size_t j = 0; j < half; j++) s += 'a' + l(rng);
        while (s.size() < len) s += '0' + d(rng);
        inputs.push_back(std::move(s));
    }
    return inputs;
}

inline std::vector<std::string> gen_json_key(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> first(0, 52), rest(0, 62);
    const char fc[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_";
    const char rc[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789";
    for (int i = 0; i < count; i++) {
        std::string s(1, fc[first(rng)]);
        while (s.size() < len) s += rc[rest(rng)];
        inputs.push_back(std::move(s));
    }
    return inputs;
}

inline std::vector<std::string> gen_http_header_full(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> l(0, 25), a(0, 61);
    const char alnum[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (int i = 0; i < count; i++) {
        size_t key_len = len / 3, val_len = len - key_len - 2;
        std::string s;
        for (size_t j = 0; j < key_len; j++) s += 'A' + l(rng);
        s += ": ";
        for (size_t j = 0; j < val_len; j++) s += alnum[a(rng)];
        inputs.push_back(std::move(s));
    }
    return inputs;
}

inline std::vector<std::string> gen_log_time_full(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> d(0, 9);
    for (int i = 0; i < count; i++) {
        std::string s;
        for (int j = 0; j < 2; j++) s += '0' + d(rng);
        s += ':';
        for (int j = 0; j < 2; j++) s += '0' + d(rng);
        s += ':';
        for (int j = 0; j < 2; j++) s += '0' + d(rng);
        while (s.size() < len) s += '0' + d(rng);
        inputs.push_back(s.substr(0, len > 8 ? len : 8));
    }
    return inputs;
}

// Fallback pattern generators
inline std::vector<std::string> gen_repeated_char(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> c(0, 25);
    for (int i = 0; i < count; i++) inputs.push_back(std::string(len, 'a' + c(rng)));
    return inputs;
}

inline std::vector<std::string> gen_lazy_match(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> l(0, 25);
    for (int i = 0; i < count; i++) {
        std::string s(len - 1, 'a');
        for (size_t j = 0; j < len - 1; j++) s[j] = 'a' + l(rng);
        s += 'x';
        inputs.push_back(std::move(s));
    }
    return inputs;
}

inline std::vector<std::string> gen_lookahead(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> l(0, 25), d(0, 9);
    for (int i = 0; i < count; i++) {
        std::string s;
        for (size_t j = 0; j + 1 < len; j += 2) { s += 'a' + l(rng); s += '0' + d(rng); }
        if (s.size() < len) s += 'a' + l(rng);
        inputs.push_back(s.substr(0, len));
    }
    return inputs;
}

inline std::vector<std::string> gen_repeated_group(size_t len, int count, unsigned int /*seed*/) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    for (int i = 0; i < count; i++) {
        std::string s;
        while (s.size() + 3 <= len) s += "abc";
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// Fixed literal generators (adversarial)
inline auto gen_literal_test(size_t, int c, unsigned int) { return gen_literal("test", c); }
inline auto gen_literal_hello_world(size_t, int c, unsigned int) { return gen_literal("hello world", c); }
inline auto gen_single_a_only(size_t, int c, unsigned int) { return gen_literal("a", c); }
inline auto gen_hello(size_t, int c, unsigned int) { return gen_literal("hello", c); }
inline auto gen_foobar(size_t, int c, unsigned int) { return gen_literal("foobar", c); }
inline auto gen_ab_only(size_t, int c, unsigned int) { return gen_literal("ab", c); }
inline auto gen_abc_only(size_t, int c, unsigned int) { return gen_literal("abc", c); }
inline auto gen_abcd_only(size_t, int c, unsigned int) { return gen_literal("abcd", c); }
inline auto gen_8char(size_t, int c, unsigned int) { return gen_literal("abcdefgh", c); }
inline auto gen_15char(size_t, int c, unsigned int) { return gen_literal("abcdefghijklmno", c); }
inline auto gen_16char(size_t, int c, unsigned int) { return gen_literal("abcdefghijklmnop", c); }
inline auto gen_20char(size_t, int c, unsigned int) { return gen_literal("abcdefghijklmnopqrst", c); }
inline auto gen_24char(size_t, int c, unsigned int) { return gen_literal("abcdefghijklmnopqrstuvwx", c); }
inline auto gen_28char(size_t, int c, unsigned int) { return gen_literal("abcdefghijklmnopqrstuvwxyzab", c); }
inline auto gen_literal_32(size_t, int c, unsigned int) { return gen_literal("abcdefghijklmnopqrstuvwxyz012345", c); }
inline auto gen_64char(size_t, int c, unsigned int) { return gen_literal("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789ab", c); }
inline auto gen_128char(size_t, int c, unsigned int) { return gen_literal("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcd", c); }
inline auto gen_256char(size_t, int c, unsigned int) { return gen_literal("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcd", c); }

// Adversarial generators
inline auto gen_bounded_short(size_t, int c, unsigned int) { return gen_literal("abc", c); }
inline auto gen_optional_a(size_t, int c, unsigned int) { return gen_literal("", c); }
inline auto gen_optional_ab(size_t, int c, unsigned int) { return gen_literal("a", c); }
inline auto gen_optional_4(size_t, int c, unsigned int) { return gen_literal("abcd", c); }
inline auto gen_single_letter(size_t, int c, unsigned int) { return gen_literal("x", c); }
inline auto gen_aa_only(size_t, int c, unsigned int) { return gen_literal("aa", c); }
inline auto gen_a_or_aa(size_t, int c, unsigned int) { return gen_literal("aa", c); }
inline auto gen_empty_or_x(size_t, int c, unsigned int) { return gen_literal("", c); }
inline auto gen_dot_only(size_t, int c, unsigned int) { return gen_literal(".", c); }
inline auto gen_a_or_b(size_t, int c, unsigned int) { return gen_literal("a", c); }
inline auto gen_ab_or_cd(size_t, int c, unsigned int) { return gen_literal("ab", c); }
inline auto gen_ac_ad_bc_bd(size_t, int c, unsigned int) { return gen_literal("ac", c); }
inline auto gen_ac_or_abc(size_t, int c, unsigned int) { return gen_literal("abc", c); }
inline auto gen_a_or_ab(size_t, int c, unsigned int) { return gen_literal("ab", c); }
inline auto gen_a_or_empty(size_t, int c, unsigned int) { return gen_literal("a", c); }
inline auto gen_any_single(size_t, int c, unsigned int) { return gen_literal("x", c); }
inline auto gen_any_two(size_t, int c, unsigned int) { return gen_literal("xy", c); }
inline auto gen_short_a(size_t, int c, unsigned int) { return gen_literal("aaa", c); }
inline auto gen_a_dot_b(size_t, int c, unsigned int) { return gen_literal("a.b", c); }
inline auto gen_axbxc(size_t, int c, unsigned int) { return gen_literal("axbxc", c); }
inline auto gen_ends_a(size_t, int c, unsigned int) { return gen_literal("xyza", c); }
inline auto gen_xy(size_t, int c, unsigned int) { return gen_literal("y", c); }
inline auto gen_aab(size_t, int c, unsigned int) { return gen_literal("aab", c); }
inline auto gen_testing(size_t, int c, unsigned int) { return gen_literal("testing", c); }
inline auto gen_aaaa(size_t, int c, unsigned int) { return gen_literal("aaaa", c); }
inline auto gen_wxyz(size_t, int c, unsigned int) { return gen_literal("wxyz", c); }
inline auto gen_1234(size_t, int c, unsigned int) { return gen_literal("1234", c); }
inline auto gen_best(size_t, int c, unsigned int) { return gen_literal("best", c); }
inline auto gen_fest(size_t, int c, unsigned int) { return gen_literal("fest", c); }
inline auto gen_rest(size_t, int c, unsigned int) { return gen_literal("rest", c); }
inline auto gen_abab(size_t, int c, unsigned int) { return gen_literal("abab", c); }
inline auto gen_aabb(size_t, int c, unsigned int) { return gen_literal("aabb", c); }

// Counted repetition generators  
inline auto gen_a_20(size_t, int c, unsigned int) { return gen_literal(std::string(20, 'a'), c); }
inline auto gen_a10(size_t, int c, unsigned int) { return gen_literal(std::string(10, 'a'), c); }
inline auto gen_a50(size_t, int c, unsigned int) { return gen_literal(std::string(50, 'a'), c); }
inline auto gen_a100(size_t, int c, unsigned int) { return gen_literal(std::string(100, 'a'), c); }

// Word alternation generators
inline auto gen_word_choice(size_t, int c, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(c);
    std::mt19937 rng(seed);
    const char* words[] = {"cat", "dog", "bird", "fish"};
    std::uniform_int_distribution<int> w(0, 3);
    for (int i = 0; i < c; i++) inputs.push_back(words[w(rng)]);
    return inputs;
}

inline auto gen_greek_word(size_t, int c, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(c);
    std::mt19937 rng(seed);
    const char* words[] = {"alpha", "beta", "gamma", "delta", "epsilon", "zeta", "eta", "theta", "iota", "kappa"};
    std::uniform_int_distribution<int> w(0, 9);
    for (int i = 0; i < c; i++) inputs.push_back(words[w(rng)]);
    return inputs;
}

inline auto gen_greek(size_t, int c, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(c);
    std::mt19937 rng(seed);
    const char* words[] = {"alpha", "beta", "gamma", "delta", "epsilon", "zeta", "eta", "theta"};
    std::uniform_int_distribution<int> w(0, 7);
    for (int i = 0; i < c; i++) inputs.push_back(words[w(rng)]);
    return inputs;
}

inline auto gen_numbers(size_t, int c, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(c);
    std::mt19937 rng(seed);
    const char* words[] = {"one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "eleven", "twelve"};
    std::uniform_int_distribution<int> w(0, 11);
    for (int i = 0; i < c; i++) inputs.push_back(words[w(rng)]);
    return inputs;
}

inline auto gen_pairs(size_t, int c, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(c);
    std::mt19937 rng(seed);
    const char* words[] = {"aa", "bb", "cc", "dd", "ee", "ff"};
    std::uniform_int_distribution<int> w(0, 5);
    for (int i = 0; i < c; i++) inputs.push_back(words[w(rng)]);
    return inputs;
}

inline auto gen_10_pairs(size_t, int c, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(c);
    std::mt19937 rng(seed);
    const char* words[] = {"aa", "bb", "cc", "dd", "ee", "ff", "gg", "hh", "ii", "jj"};
    std::uniform_int_distribution<int> w(0, 9);
    for (int i = 0; i < c; i++) inputs.push_back(words[w(rng)]);
    return inputs;
}

inline auto gen_4x3(size_t, int c, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(c);
    std::mt19937 rng(seed);
    const char* words[] = {"foo", "bar", "baz", "qux"};
    std::uniform_int_distribution<int> w(0, 3);
    for (int i = 0; i < c; i++) inputs.push_back(words[w(rng)]);
    return inputs;
}

inline auto gen_5_words(size_t, int c, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(c);
    std::mt19937 rng(seed);
    const char* words[] = {"the", "quick", "brown", "fox", "jumps"};
    std::uniform_int_distribution<int> w(0, 4);
    for (int i = 0; i < c; i++) inputs.push_back(words[w(rng)]);
    return inputs;
}

inline auto gen_alt_16(size_t, int c, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(c);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> w(0, 15);
    for (int i = 0; i < c; i++) inputs.push_back(std::string(1, 'a' + w(rng)));
    return inputs;
}

inline auto gen_12x3(size_t, int c, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(c);
    std::mt19937 rng(seed);
    const char* words[] = {"abc", "def", "ghi", "jkl", "mno", "pqr", "stu", "vwx", "yz0", "123", "456", "789"};
    std::uniform_int_distribution<int> w(0, 11);
    for (int i = 0; i < c; i++) inputs.push_back(words[w(rng)]);
    return inputs;
}

inline auto gen_varied_alt(size_t, int c, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(c);
    std::mt19937 rng(seed);
    const char* words[] = {"a", "bb", "ccc", "dddd", "eeeee"};
    std::uniform_int_distribution<int> w(0, 4);
    for (int i = 0; i < c; i++) inputs.push_back(words[w(rng)]);
    return inputs;
}

// Complex pattern generators
inline auto gen_optional_www(size_t len, int c, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(c);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> b(0, 1);
    for (int i = 0; i < c; i++) inputs.push_back(b(rng) ? "www.example" : "example");
    return inputs;
}

inline auto gen_contains_middle(size_t len, int c, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(c);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> l(0, 25);
    for (int i = 0; i < c; i++) {
        std::string s;
        size_t pre = len / 3;
        for (size_t j = 0; j < pre; j++) s += 'a' + l(rng);
        s += "middle";
        while (s.size() < len) s += 'a' + l(rng);
        inputs.push_back(std::move(s));
    }
    return inputs;
}

inline auto gen_prefix_digits(size_t len, int c, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(c);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> d(0, 9);
    for (int i = 0; i < c; i++) {
        std::string s = "id:";
        while (s.size() < len) s += '0' + d(rng);
        inputs.push_back(std::move(s));
    }
    return inputs;
}

inline auto gen_nested_optional(size_t, int c, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(c);
    std::mt19937 rng(seed);
    const char* variants[] = {"d", "cd", "bcd", "abcd"};
    std::uniform_int_distribution<int> v(0, 3);
    for (int i = 0; i < c; i++) inputs.push_back(variants[v(rng)]);
    return inputs;
}

inline auto gen_data_suffix(size_t, int c, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(c);
    std::mt19937 rng(seed);
    const char* suffixes[] = {"data_one", "data_two", "data_three", "data_four", "data_five"};
    std::uniform_int_distribution<int> s(0, 4);
    for (int i = 0; i < c; i++) inputs.push_back(suffixes[s(rng)]);
    return inputs;
}

inline auto gen_interleaved(size_t, int c, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(c);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> x(0, 25);
    for (int i = 0; i < c; i++) {
        std::string s;
        for (char ch = 'a'; ch <= 'h'; ch++) {
            s += ch;
            if (ch < 'h') s += 'a' + x(rng);
        }
        inputs.push_back(std::move(s));
    }
    return inputs;
}

inline auto gen_optional_6(size_t, int c, unsigned int) { return gen_literal("abcdef", c); }
inline auto gen_abcde(size_t, int c, unsigned int) { return gen_literal("abcde", c); }
inline auto gen_abcdef(size_t, int c, unsigned int) { return gen_literal("abcdef", c); }
inline auto gen_abcdefgh(size_t, int c, unsigned int) { return gen_literal("abcdefgh", c); }

inline auto gen_foo_xxx_bar(size_t, int c, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(c);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> l(0, 25);
    for (int i = 0; i < c; i++) {
        std::string s = "foo";
        for (int j = 0; j < 3; j++) s += 'a' + l(rng);
        s += "bar";
        inputs.push_back(std::move(s));
    }
    return inputs;
}

inline auto gen_start_range_end(size_t, int c, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(c);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> l(0, 25);
    for (int i = 0; i < c; i++) {
        std::string s = "start";
        for (int j = 0; j < 5; j++) s += 'a' + l(rng);
        s += "end";
        inputs.push_back(std::move(s));
    }
    return inputs;
}

inline auto gen_dot_sep_6(size_t, int c, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(c);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> x(0, 25);
    for (int i = 0; i < c; i++) {
        std::string s;
        for (char ch = 'a'; ch <= 'f'; ch++) {
            s += ch;
            if (ch < 'f') s += 'a' + x(rng);
        }
        inputs.push_back(std::move(s));
    }
    return inputs;
}

inline auto gen_dot_sep_10(size_t, int c, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(c);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> x(0, 25);
    for (int i = 0; i < c; i++) {
        std::string s;
        for (char ch = 'a'; ch <= 'j'; ch++) {
            s += ch;
            if (ch < 'j') s += 'a' + x(rng);
        }
        inputs.push_back(std::move(s));
    }
    return inputs;
}

inline auto gen_dot_chain_12(size_t, int c, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(c);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> x(0, 25);
    for (int i = 0; i < c; i++) {
        std::string s;
        for (char ch = 'a'; ch <= 'l'; ch++) {
            s += ch;
            if (ch < 'l') s += 'a' + x(rng);
        }
        inputs.push_back(std::move(s));
    }
    return inputs;
}

inline auto gen_alt_seq_4(size_t, int c, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(c);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> b(0, 1);
    for (int i = 0; i < c; i++) {
        std::string s;
        s += b(rng) ? 'a' : 'b';
        s += b(rng) ? 'c' : 'd';
        s += b(rng) ? 'e' : 'f';
        s += b(rng) ? 'g' : 'h';
        inputs.push_back(std::move(s));
    }
    return inputs;
}

inline auto gen_a1a1a(size_t, int c, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(c);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> l(0, 25), d(0, 9);
    for (int i = 0; i < c; i++) {
        std::string s;
        s += 'a' + l(rng); s += '0' + d(rng);
        s += 'a' + l(rng); s += '0' + d(rng);
        s += 'a' + l(rng);
        inputs.push_back(std::move(s));
    }
    return inputs;
}

inline auto gen_id_name(size_t, int c, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(c);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> l(0, 25), d(0, 9);
    for (int i = 0; i < c; i++) {
        std::string s = "id_";
        for (int j = 0; j < 3; j++) s += '0' + d(rng);
        s += "_name_";
        for (int j = 0; j < 5; j++) s += 'a' + l(rng);
        inputs.push_back(std::move(s));
    }
    return inputs;
}

inline auto gen_class_55(size_t, int c, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(c);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> l(0, 25), d(0, 9);
    for (int i = 0; i < c; i++) {
        std::string s;
        for (int j = 0; j < 5; j++) s += 'a' + l(rng);
        for (int j = 0; j < 5; j++) s += '0' + d(rng);
        inputs.push_back(std::move(s));
    }
    return inputs;
}

inline auto gen_lit_dot_lit(size_t, int c, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(c);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> x(0, 25);
    for (int i = 0; i < c; i++) {
        std::string s = "ab";
        s += 'a' + x(rng);
        s += "cd";
        s += 'a' + x(rng);
        s += "ef";
        s += 'a' + x(rng);
        s += "gh";
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// Long range generators
inline auto gen_lower64(size_t, int c, unsigned int seed) { return gen_letters(64, c, seed); }
inline auto gen_lower256(size_t, int c, unsigned int seed) { return gen_letters(256, c, seed); }
inline auto gen_digits100(size_t, int c, unsigned int seed) { return gen_digits(100, c, seed); }
inline auto gen_digits500(size_t, int c, unsigned int seed) { return gen_digits(500, c, seed); }
inline auto gen_alpha1000(size_t, int c, unsigned int seed) { return gen_alnum(1000, c, seed); }
inline auto gen_alnum50(size_t, int c, unsigned int seed) { return gen_alnum(50, c, seed); }

inline auto gen_alt_types(size_t, int c, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(c);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> t(0, 2), l(0, 25), d(0, 9), u(0, 25);
    for (int i = 0; i < c; i++) {
        std::string s;
        int type = t(rng);
        for (int j = 0; j < 10; j++) {
            if (type == 0) s += 'a' + l(rng);
            else if (type == 1) s += '0' + d(rng);
            else s += 'A' + u(rng);
        }
        inputs.push_back(std::move(s));
    }
    return inputs;
}

inline auto gen_triple_range(size_t, int c, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(c);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> l(0, 25), d(0, 9), u(0, 25);
    for (int i = 0; i < c; i++) {
        std::string s;
        for (int j = 0; j < 20; j++) s += 'a' + l(rng);
        for (int j = 0; j < 20; j++) s += '0' + d(rng);
        for (int j = 0; j < 20; j++) s += 'A' + u(rng);
        inputs.push_back(std::move(s));
    }
    return inputs;
}

inline auto gen_lit_range_lit_big(size_t, int c, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(c);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> l(0, 25);
    for (int i = 0; i < c; i++) {
        std::string s = "prefix_";
        for (int j = 0; j < 30; j++) s += 'a' + l(rng);
        s += "_suffix";
        inputs.push_back(std::move(s));
    }
    return inputs;
}

} // namespace bench
