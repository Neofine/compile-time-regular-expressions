#pragma once
// ============================================================================
// PATTERN REGISTRY - Input generators for benchmarking
// ============================================================================
//
// Match rates:
// - Simple/Complex/Scaling/RealWorld: 100% matching (measures match speed)
// - NonMatch: 0% matching (measures rejection speed)
// - Fallback: 50% matching (tests mixed behavior)

#include <functional>
#include <random>
#include <string>
#include <vector>

namespace bench {

// ============================================================================
// INPUT GENERATORS
// ============================================================================

// Generator function type: (length, count, seed) -> vector of strings
using InputGenerator = std::function<std::vector<std::string>(size_t, int, unsigned int)>;

// Digits [0-9]+ : 100% matching (all digits)
inline std::vector<std::string> gen_digits(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> d(0, 9);

    for (int i = 0; i < count; i++) {
        std::string s(len, '0');
        for (size_t j = 0; j < len; j++)
            s[j] = '0' + d(rng);
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// Letters [a-z]+ : 100% matching (all letters)
inline std::vector<std::string> gen_letters(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> l(0, 25);

    for (int i = 0; i < count; i++) {
        std::string s(len, 'a');
        for (size_t j = 0; j < len; j++)
            s[j] = 'a' + l(rng);
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// Pure letters [a-z]+ : 100% all letters (for non-match testing)
inline std::vector<std::string> gen_pure_letters(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> l(0, 25);

    for (int i = 0; i < count; i++) {
        std::string s(len, 'a');
        for (size_t j = 0; j < len; j++)
            s[j] = 'a' + l(rng);
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// Pure digits [0-9]+ : 100% all digits (for non-match testing)
inline std::vector<std::string> gen_pure_digits(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> d(0, 9);

    for (int i = 0; i < count; i++) {
        std::string s(len, '0');
        for (size_t j = 0; j < len; j++)
            s[j] = '0' + d(rng);
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// Letters without specific literal (for testing dominator prefilter rejection)
// Generates strings that contain NO occurrence of the target literal
inline std::vector<std::string> gen_no_test_literal(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    // Use only letters that can't form "test" - exclude 't', 'e', 's'
    const char safe_chars[] = "abcdfghijklmnopqruvwxyz";
    std::uniform_int_distribution<int> d(0, sizeof(safe_chars) - 2);

    for (int i = 0; i < count; i++) {
        std::string s(len, 'a');
        for (size_t j = 0; j < len; j++)
            s[j] = safe_chars[d(rng)];
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// Letters without "http" (for URL pattern prefilter testing)
inline std::vector<std::string> gen_no_http_literal(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    // Use only letters that can't form "http" - exclude 'h', 't', 'p'
    const char safe_chars[] = "abcdefgijklmnoqrsuvwxyz";
    std::uniform_int_distribution<int> d(0, sizeof(safe_chars) - 2);

    for (int i = 0; i < count; i++) {
        std::string s(len, 'a');
        for (size_t j = 0; j < len; j++)
            s[j] = safe_chars[d(rng)];
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// Letters without common suffix "ing" (for region analysis prefilter testing)
inline std::vector<std::string> gen_no_ing_suffix(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    // Use only letters that can't form "ing" - exclude 'i', 'n', 'g'
    const char safe_chars[] = "abcdefhjklmopqrstuvwxyz";
    std::uniform_int_distribution<int> d(0, sizeof(safe_chars) - 2);

    for (int i = 0; i < count; i++) {
        std::string s(len, 'a');
        for (size_t j = 0; j < len; j++)
            s[j] = safe_chars[d(rng)];
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// Vowels [aeiou]+ : 100% matching (all vowels)
inline std::vector<std::string> gen_vowels(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    const char vowels[] = {'a', 'e', 'i', 'o', 'u'};
    std::uniform_int_distribution<int> v(0, 4);

    for (int i = 0; i < count; i++) {
        std::string s(len, 'a');
        for (size_t j = 0; j < len; j++)
            s[j] = vowels[v(rng)];
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// Hex [0-9a-fA-F]+ : 100% matching (all hex)
inline std::vector<std::string> gen_hex(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    const char hex_chars[] = "0123456789abcdefABCDEF";
    std::uniform_int_distribution<int> h(0, 21);

    for (int i = 0; i < count; i++) {
        std::string s(len, '0');
        for (size_t j = 0; j < len; j++)
            s[j] = hex_chars[h(rng)];
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// Alphanumeric [a-zA-Z0-9]+ : 100% matching (all alphanumeric)
inline std::vector<std::string> gen_alnum(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    const char alnum[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::uniform_int_distribution<int> a(0, 61);

    for (int i = 0; i < count; i++) {
        std::string s(len, 'a');
        for (size_t j = 0; j < len; j++)
            s[j] = alnum[a(rng)];
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// Whitespace [ \t\n\r]+ : 50% valid, 50% end with letter
inline std::vector<std::string> gen_whitespace(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    const char ws[] = " \t\n\r";
    std::uniform_int_distribution<int> w(0, 3);
    std::uniform_int_distribution<int> l(0, 25);

    for (int i = 0; i < count; i++) {
        std::string s(len, ' ');
        for (size_t j = 0; j < len - 1; j++)
            s[j] = ws[w(rng)];
        s[len - 1] = (i % 2 == 0) ? ws[w(rng)] : ('a' + l(rng));
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// Word characters [a-zA-Z_]+ : 50% valid, 50% end with digit
inline std::vector<std::string> gen_word(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    const char word[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_";
    std::uniform_int_distribution<int> w(0, 52);
    std::uniform_int_distribution<int> d(0, 9);

    for (int i = 0; i < count; i++) {
        std::string s(len, 'a');
        for (size_t j = 0; j < len - 1; j++)
            s[j] = word[w(rng)];
        s[len - 1] = (i % 2 == 0) ? word[w(rng)] : ('0' + d(rng));
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// Single char a+ : 50% all 'a', 50% end with 'b'
inline std::vector<std::string> gen_single_a(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);

    for (int i = 0; i < count; i++) {
        std::string s(len, 'a');
        if (i % 2 == 1)
            s[len - 1] = 'b';
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// ============================================================================
// SEQUENCE FUSION PATTERNS - Literals + Character Classes
// ============================================================================

// Fixed literal "hello" : 50% match, 50% one char off
inline std::vector<std::string> gen_literal_hello(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);

    // For this pattern, we need exactly "hello" (5 chars)
    // If len != 5, we pad or truncate
    const char* literal = "hello";
    size_t lit_len = 5;

    for (int i = 0; i < count; i++) {
        std::string s;
        if (len >= lit_len) {
            s = std::string(literal, lit_len);
            s += std::string(len - lit_len, 'o');  // Pad with 'o'
        } else {
            s = std::string(literal, len);
        }

        // 50% invalid: change one character
        if (i % 2 == 1) {
            s[len/2] = 'X';
        }
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// Digit-dot-digit pattern [0-9]+\.[0-9]+ : 100% matching
inline std::vector<std::string> gen_decimal(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> d(0, 9);

    for (int i = 0; i < count; i++) {
        std::string s;
        size_t half = len / 2;

        // First part: digits
        for (size_t j = 0; j < half; j++)
            s += ('0' + d(rng));

        // Dot
        s += '.';

        // Second part: digits (fill remaining)
        while (s.length() < len)
            s += ('0' + d(rng));

        inputs.push_back(std::move(s));
    }
    return inputs;
}

// IPv4-like: [0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}
inline std::vector<std::string> gen_ipv4_like(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> octet(0, 255);

    for (int i = 0; i < count; i++) {
        // Generate valid IPv4
        std::string s = std::to_string(octet(rng)) + "." +
                       std::to_string(octet(rng)) + "." +
                       std::to_string(octet(rng)) + "." +
                       std::to_string(octet(rng));

        // Pad or truncate to length
        if (s.length() < len) {
            // Pad with valid pattern repetition
            while (s.length() < len) {
                s += "." + std::to_string(octet(rng));
            }
            s = s.substr(0, len);
        } else if (s.length() > len) {
            s = s.substr(0, len);
        }

        // 50% invalid: replace a dot with letter
        if (i % 2 == 1 && s.find('.') != std::string::npos) {
            size_t dot_pos = s.find('.');
            s[dot_pos] = 'x';
        }
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// MAC-like: [0-9a-fA-F]{2}:[0-9a-fA-F]{2}:...
inline std::vector<std::string> gen_mac_like(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    const char hex[] = "0123456789abcdef";
    std::uniform_int_distribution<int> h(0, 15);

    for (int i = 0; i < count; i++) {
        std::string s;
        size_t pos = 0;

        while (s.length() < len) {
            // Add hex pair
            if (s.length() + 2 <= len) {
                s += hex[h(rng)];
                s += hex[h(rng)];
            }
            // Add colon if not at end
            if (s.length() < len && s.length() + 1 <= len) {
                s += ':';
            }
        }
        s = s.substr(0, len);

        // 50% invalid: replace colon with 'g'
        if (i % 2 == 1 && s.find(':') != std::string::npos) {
            size_t colon_pos = s.find(':');
            s[colon_pos] = 'g';
        }
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// Date-like: [0-9]{4}-[0-9]{2}-[0-9]{2}
inline std::vector<std::string> gen_date_like(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> year(1900, 2100);
    std::uniform_int_distribution<int> month(1, 12);
    std::uniform_int_distribution<int> day(1, 28);

    for (int i = 0; i < count; i++) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%04d-%02d-%02d", year(rng), month(rng), day(rng));
        std::string s(buf);

        // Pad or truncate
        while (s.length() < len) {
            snprintf(buf, sizeof(buf), "-%02d", day(rng));
            s += buf;
        }
        s = s.substr(0, len);

        // 50% invalid: replace dash with letter
        if (i % 2 == 1 && s.find('-') != std::string::npos) {
            size_t dash_pos = s.find('-');
            s[dash_pos] = 'x';
        }
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// Negated pattern [^0-9]+ : all letters, 50% end with digit
inline std::vector<std::string> gen_negated_digits(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> l(0, 25);
    std::uniform_int_distribution<int> d(0, 9);

    for (int i = 0; i < count; i++) {
        std::string s(len, 'a');
        for (size_t j = 0; j < len - 1; j++)
            s[j] = 'a' + l(rng);
        s[len - 1] = (i % 2 == 0) ? ('a' + l(rng)) : ('0' + d(rng));
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// ============================================================================
// ADDITIONAL PATTERNS - Testing more code paths
// ============================================================================

// Dot star .*x : 50% end with 'x', 50% end with 'y'
inline std::vector<std::string> gen_dot_star_x(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> c(32, 126);  // Printable ASCII

    for (int i = 0; i < count; i++) {
        std::string s(len, 'a');
        for (size_t j = 0; j < len - 1; j++)
            s[j] = (char)c(rng);
        s[len - 1] = (i % 2 == 0) ? 'x' : 'y';
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// Mixed pattern [a-z]+[0-9]+ : 100% matching (letters then digits)
inline std::vector<std::string> gen_letters_then_digits(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> l(0, 25);
    std::uniform_int_distribution<int> d(0, 9);

    for (int i = 0; i < count; i++) {
        std::string s(len, 'a');
        size_t split = len / 2;

        for (size_t j = 0; j < split; j++)
            s[j] = 'a' + l(rng);

        for (size_t j = split; j < len; j++)
            s[j] = '0' + d(rng);

        inputs.push_back(std::move(s));
    }
    return inputs;
}

// Alternation (cat|dog|fish)+ : 50% repeating valid, 50% invalid word
inline std::vector<std::string> gen_alternation(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    const char* words[] = {"cat", "dog"};  // "fish" is too long for short inputs
    std::uniform_int_distribution<int> w(0, 1);

    for (int i = 0; i < count; i++) {
        std::string s;
        if (i % 2 == 0) {
            // Valid: repeating cat/dog
            while (s.length() < len) {
                s += words[w(rng)];
            }
            s = s.substr(0, len);
            // Ensure it ends at a word boundary or pad
            size_t rem = len % 3;
            if (rem != 0) {
                s = s.substr(0, len - rem);
                while (s.length() < len) s += words[w(rng)][0];
            }
        } else {
            // Invalid: contains 'x'
            s = std::string(len, 'c');
            s[len/2] = 'x';
        }
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// Upper case [A-Z]+ : 100% matching (all uppercase)
inline std::vector<std::string> gen_upper(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> u(0, 25);

    for (int i = 0; i < count; i++) {
        std::string s(len, 'A');
        for (size_t j = 0; j < len; j++)
            s[j] = 'A' + u(rng);
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// Binary [01]+ : 50% valid, 50% contains '2'
inline std::vector<std::string> gen_binary(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> b(0, 1);

    for (int i = 0; i < count; i++) {
        std::string s(len, '0');
        for (size_t j = 0; j < len - 1; j++)
            s[j] = '0' + b(rng);
        s[len - 1] = (i % 2 == 0) ? ('0' + b(rng)) : '2';
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// [ab]+ : strings of only 'a' and 'b'
inline std::vector<std::string> gen_ab(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> d(0, 1);

    for (int i = 0; i < count; i++) {
        std::string s(len, 'a');
        for (size_t j = 0; j < len; j++)
            s[j] = 'a' + d(rng);
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// [abcd]+ : strings of only 'a', 'b', 'c', 'd'
inline std::vector<std::string> gen_abcd(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> d(0, 3);

    for (int i = 0; i < count; i++) {
        std::string s(len, 'a');
        for (size_t j = 0; j < len; j++)
            s[j] = 'a' + d(rng);
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// Full-match version of IPv4: pattern [0-9]+\.[0-9]+\.[0-9]+\.[0-9]+
// Generates strings like "123.456.789.012" padded to fit len
inline std::vector<std::string> gen_ipv4_full(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> digit(0, 9);

    for (int i = 0; i < count; i++) {
        // Split len into 4 parts separated by 3 dots
        size_t part_len = (len - 3) / 4;  // -3 for the dots
        std::string s;
        for (int p = 0; p < 4; p++) {
            if (p > 0) s += '.';
            size_t plen = (p == 3) ? (len - s.length()) : part_len;
            for (size_t j = 0; j < plen; j++) {
                s += '0' + digit(rng);
            }
        }
        inputs.push_back(s.substr(0, len));
    }
    return inputs;
}

// Full-match email: "user@domain.com" format
inline std::vector<std::string> gen_email_full(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> letter(0, 25);

    for (int i = 0; i < count; i++) {
        // user@domain.tld format
        std::string s;
        size_t user_len = len / 3;
        size_t domain_len = len / 3;
        size_t tld_len = len - user_len - domain_len - 2; // -2 for @ and .

        for (size_t j = 0; j < user_len; j++) s += 'a' + letter(rng);
        s += '@';
        for (size_t j = 0; j < domain_len; j++) s += 'a' + letter(rng);
        s += '.';
        for (size_t j = 0; j < tld_len && s.length() < len; j++) s += 'a' + letter(rng);

        inputs.push_back(s.substr(0, len));
    }
    return inputs;
}

// Full-match date: "2024-01-15" or similar with only digits and dashes
inline std::vector<std::string> gen_date_full(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> digit(0, 9);

    for (int i = 0; i < count; i++) {
        std::string s;
        // Pattern: [0-9]+-[0-9]+-[0-9]+
        size_t part = (len - 2) / 3; // -2 for two dashes
        for (size_t j = 0; j < part; j++) s += '0' + digit(rng);
        s += '-';
        for (size_t j = 0; j < part; j++) s += '0' + digit(rng);
        s += '-';
        while (s.length() < len) s += '0' + digit(rng);
        inputs.push_back(s.substr(0, len));
    }
    return inputs;
}

// Full-match log time: "12:34:56" format
inline std::vector<std::string> gen_log_time_full(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> digit(0, 9);

    for (int i = 0; i < count; i++) {
        std::string s;
        // Pattern: [0-9]+:[0-9]+:[0-9]+
        size_t part = (len - 2) / 3;
        for (size_t j = 0; j < part; j++) s += '0' + digit(rng);
        s += ':';
        for (size_t j = 0; j < part; j++) s += '0' + digit(rng);
        s += ':';
        while (s.length() < len) s += '0' + digit(rng);
        inputs.push_back(s.substr(0, len));
    }
    return inputs;
}

// Full-match HTTP header: "Content-Type: application"
inline std::vector<std::string> gen_http_header_full(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> letter(0, 25);

    for (int i = 0; i < count; i++) {
        std::string s;
        size_t key_len = len / 3;
        // Key part (letters and dashes)
        for (size_t j = 0; j < key_len; j++) {
            s += (j % 5 == 4) ? '-' : ('A' + letter(rng));
        }
        s += ": ";
        // Value part (alphanumeric and spaces)
        while (s.length() < len) {
            char c = 'a' + letter(rng);
            s += c;
        }
        inputs.push_back(s.substr(0, len));
    }
    return inputs;
}

// Negated vowels [^aeiou]+ : consonants only, 50% end with vowel
inline std::vector<std::string> gen_negated_vowels(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    const char consonants[] = "bcdfghjklmnpqrstvwxyz";
    const char vowels[] = "aeiou";
    std::uniform_int_distribution<int> c(0, 20);
    std::uniform_int_distribution<int> v(0, 4);

    for (int i = 0; i < count; i++) {
        std::string s(len, 'b');
        for (size_t j = 0; j < len - 1; j++)
            s[j] = consonants[c(rng)];
        s[len - 1] = (i % 2 == 0) ? consonants[c(rng)] : vowels[v(rng)];
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// Extended ASCII [\\x20-\\x7e]+ (printable): 50% valid, 50% has control char
inline std::vector<std::string> gen_printable(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> p(0x20, 0x7e);

    for (int i = 0; i < count; i++) {
        std::string s(len, ' ');
        for (size_t j = 0; j < len - 1; j++)
            s[j] = (char)p(rng);
        s[len - 1] = (i % 2 == 0) ? (char)p(rng) : '\x01';
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// ============================================================================
// REAL-WORLD PATTERNS - Network, Logs, Data Formats
// ============================================================================

// IPv4-like: [0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}
// Generates: "192.168.1.100" style strings, padded to length
inline std::vector<std::string> gen_ipv4(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> octet(0, 255);

    for (int i = 0; i < count; i++) {
        std::string s;
        // Generate repeating IPv4 addresses to fill length
        while (s.length() < len) {
            if (!s.empty()) s += ".";
            s += std::to_string(octet(rng)) + "." +
                 std::to_string(octet(rng)) + "." +
                 std::to_string(octet(rng)) + "." +
                 std::to_string(octet(rng));
        }
        s = s.substr(0, len);

        // 50% invalid: replace a dot with letter
        if (i % 2 == 1 && s.find('.') != std::string::npos) {
            size_t dot_pos = s.find('.');
            s[dot_pos] = 'x';
        }
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// UUID-like: [0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}
// Fixed length: 36 chars. Pad/truncate as needed.
inline std::vector<std::string> gen_uuid(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    const char hex[] = "0123456789abcdef";
    std::uniform_int_distribution<int> h(0, 15);

    for (int i = 0; i < count; i++) {
        std::string s;
        // Generate repeating UUIDs to fill length
        while (s.length() < len) {
            if (!s.empty()) s += "-";
            // 8-4-4-4-12 format
            for (int j = 0; j < 8; j++) s += hex[h(rng)];
            s += "-";
            for (int j = 0; j < 4; j++) s += hex[h(rng)];
            s += "-";
            for (int j = 0; j < 4; j++) s += hex[h(rng)];
            s += "-";
            for (int j = 0; j < 4; j++) s += hex[h(rng)];
            s += "-";
            for (int j = 0; j < 12; j++) s += hex[h(rng)];
        }
        s = s.substr(0, len);

        // 50% invalid: replace hex with 'g'
        if (i % 2 == 1) {
            size_t pos = rng() % len;
            while (s[pos] == '-') pos = (pos + 1) % len;
            s[pos] = 'g';
        }
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// Full-match UUID pattern: [0-9a-f]+-[0-9a-f]+-[0-9a-f]+-[0-9a-f]+-[0-9a-f]+
// 5 hex groups separated by 4 dashes
inline std::vector<std::string> gen_uuid_full(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    const char hex[] = "0123456789abcdef";
    std::uniform_int_distribution<int> h(0, 15);

    for (int i = 0; i < count; i++) {
        // Split len into 5 parts separated by 4 dashes
        size_t part_len = (len - 4) / 5;  // -4 for the dashes
        std::string s;
        for (int p = 0; p < 5; p++) {
            if (p > 0) s += '-';
            size_t plen = (p == 4) ? (len - s.length()) : part_len;
            for (size_t j = 0; j < plen; j++) {
                s += hex[h(rng)];
            }
        }
        inputs.push_back(s.substr(0, len));
    }
    return inputs;
}

// HTTP header line: [A-Za-z-]+: [^\r\n]+
// Generates: "Content-Type: application/json" style
inline std::vector<std::string> gen_http_header(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    const char* headers[] = {"Content-Type", "Accept", "User-Agent", "Host", "Cookie"};
    const char* values[] = {"application/json", "text/html", "Mozilla/5.0", "example.com", "session=abc123"};
    std::uniform_int_distribution<int> idx(0, 4);
    std::uniform_int_distribution<int> alpha(0, 51);
    const char alphachars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

    for (int i = 0; i < count; i++) {
        std::string s = std::string(headers[idx(rng)]) + ": " + values[idx(rng)];
        // Pad to length with random chars
        while (s.length() < len) {
            s += alphachars[alpha(rng)];
        }
        s = s.substr(0, len);

        // 50% invalid: add \r or \n in value
        if (i % 2 == 1 && s.length() > 5) {
            s[s.length()/2] = '\r';
        }
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// Log timestamp: [0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2}
// Generates: "2024-01-15 12:30:45" style (19 chars base)
inline std::vector<std::string> gen_log_timestamp(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> year(2020, 2025);
    std::uniform_int_distribution<int> month(1, 12);
    std::uniform_int_distribution<int> day(1, 28);
    std::uniform_int_distribution<int> hour(0, 23);
    std::uniform_int_distribution<int> minorsec(0, 59);

    for (int i = 0; i < count; i++) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
                 year(rng), month(rng), day(rng), hour(rng), minorsec(rng), minorsec(rng));
        std::string s(buf);

        // Pad with more timestamps
        while (s.length() < len) {
            snprintf(buf, sizeof(buf), " %04d-%02d-%02d %02d:%02d:%02d",
                     year(rng), month(rng), day(rng), hour(rng), minorsec(rng), minorsec(rng));
            s += buf;
        }
        s = s.substr(0, len);

        // 50% invalid: replace dash with letter
        if (i % 2 == 1 && s.find('-') != std::string::npos) {
            s[s.find('-')] = 'x';
        }
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// Email-like: [a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+
// Simplified pattern without TLD validation
inline std::vector<std::string> gen_email(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    const char localchars[] = "abcdefghijklmnopqrstuvwxyz0123456789._%+-";
    const char domainchars[] = "abcdefghijklmnopqrstuvwxyz0123456789.-";
    std::uniform_int_distribution<int> local_idx(0, sizeof(localchars) - 2);
    std::uniform_int_distribution<int> domain_idx(0, sizeof(domainchars) - 2);

    for (int i = 0; i < count; i++) {
        std::string s;
        size_t at_pos = len / 3;  // @ at 1/3 of length

        // Local part
        for (size_t j = 0; j < at_pos && s.length() < len; j++) {
            s += localchars[local_idx(rng)];
        }
        s += '@';

        // Domain part
        while (s.length() < len) {
            s += domainchars[domain_idx(rng)];
        }
        s = s.substr(0, len);

        // 50% invalid: remove @ or add invalid char
        if (i % 2 == 1) {
            size_t at = s.find('@');
            if (at != std::string::npos) {
                s[at] = '!';  // Invalid char
            }
        }
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// HTML tag-like: <[a-zA-Z][a-zA-Z0-9]*[^>]*>
// Generates: "<div class='foo'>" style
inline std::vector<std::string> gen_html_tag(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    const char* tags[] = {"div", "span", "p", "a", "img", "input", "button"};
    const char* attrs[] = {"class='foo'", "id='bar'", "style='color:red'", "href='#'", "src='img.png'"};
    std::uniform_int_distribution<int> tag_idx(0, 6);
    std::uniform_int_distribution<int> attr_idx(0, 4);
    std::uniform_int_distribution<int> attr_count(0, 3);

    for (int i = 0; i < count; i++) {
        std::string s = "<" + std::string(tags[tag_idx(rng)]);
        int nattrs = attr_count(rng);
        for (int a = 0; a < nattrs; a++) {
            s += " " + std::string(attrs[attr_idx(rng)]);
        }
        s += ">";

        // Pad with more tags
        while (s.length() < len) {
            s += "<" + std::string(tags[tag_idx(rng)]);
            int na = attr_count(rng);
            for (int a = 0; a < na; a++) {
                s += " " + std::string(attrs[attr_idx(rng)]);
            }
            s += ">";
        }
        s = s.substr(0, len);

        // 50% invalid: unclosed tag or bad first char
        if (i % 2 == 1) {
            if (s.length() > 2) {
                s[1] = '5';  // Tag can't start with digit
            }
        }
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// JSON-like key: "[a-zA-Z_][a-zA-Z0-9_]*" : 100% matching
inline std::vector<std::string> gen_json_key(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    const char first_chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_";
    const char rest_chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
    std::uniform_int_distribution<int> first_idx(0, sizeof(first_chars) - 2);
    std::uniform_int_distribution<int> rest_idx(0, sizeof(rest_chars) - 2);

    for (int i = 0; i < count; i++) {
        std::string s;
        s += first_chars[first_idx(rng)];
        while (s.length() < len) {
            s += rest_chars[rest_idx(rng)];
        }
        s = s.substr(0, len);
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// HTTP method + path: (GET|POST)/[a-z]+ : 100% matching
inline std::vector<std::string> gen_http_method(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    const char* methods[] = {"GET", "POST"};
    std::uniform_int_distribution<int> method_idx(0, 1);
    std::uniform_int_distribution<int> letter(0, 25);

    for (int i = 0; i < count; i++) {
        std::string s = methods[method_idx(rng)];
        s += '/';
        while (s.length() < len) {
            s += 'a' + letter(rng);
        }
        s = s.substr(0, len);
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// Simple URL: http://[a-z]+ : 100% matching
inline std::vector<std::string> gen_url(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> letter(0, 25);

    for (int i = 0; i < count; i++) {
        std::string s = "http://";
        while (s.length() < len) {
            s += 'a' + letter(rng);
        }
        s = s.substr(0, len);
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// Key=value format: [a-z]+=[0-9]+ : 100% matching
inline std::vector<std::string> gen_key_value(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> letter(0, 25);
    std::uniform_int_distribution<int> digit(0, 9);

    for (int i = 0; i < count; i++) {
        std::string s;
        size_t eq_pos = len / 2;

        // Key part (letters)
        for (size_t j = 0; j < eq_pos && s.length() < len; j++) {
            s += 'a' + letter(rng);
        }
        s += '=';

        // Value part (digits)
        while (s.length() < len) {
            s += '0' + digit(rng);
        }
        s = s.substr(0, len);
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// ============================================================================
// PATTERN DEFINITION
// ============================================================================

struct PatternDef {
    std::string name;           // Display name (e.g., "[0-9]+")
    std::string re2_pattern;    // Pattern for RE2/PCRE2/Hyperscan (without anchors)
    InputGenerator generator;   // Function to generate test inputs
    std::string description;    // Optional description
};

// ============================================================================
// PATTERN REGISTRY - Add new patterns here!
// ============================================================================

inline std::vector<PatternDef> get_all_patterns() {
    return {
        // Basic character classes
        {"[0-9]+", "[0-9]+", gen_digits, "Digit repetition"},
        {"[a-z]+", "[a-z]+", gen_letters, "Lowercase letter repetition"},
        {"[A-Z]+", "[A-Z]+", gen_upper, "Uppercase letter repetition"},
        {"[aeiou]+", "[aeiou]+", gen_vowels, "Vowel repetition (sparse set)"},

        // Multi-range patterns
        {"[0-9a-fA-F]+", "[0-9a-fA-F]+", gen_hex, "Hexadecimal"},
        {"[a-zA-Z0-9]+", "[a-zA-Z0-9]+", gen_alnum, "Alphanumeric"},
        {"[a-zA-Z_]+", "[a-zA-Z_]+", gen_word, "Word characters"},

        // Single character
        {"a+", "a+", gen_single_a, "Single character repetition"},

        // Whitespace
        {"[ \\t\\n\\r]+", "[ \\t\\n\\r]+", gen_whitespace, "Whitespace"},

        // Negated patterns
        {"[^0-9]+", "[^0-9]+", gen_negated_digits, "Negated digits"},
        {"[^aeiou]+", "[^aeiou]+", gen_negated_vowels, "Negated vowels"},

        // Sequence patterns (literals + character classes)
        {"[0-9]+\\.[0-9]+", "[0-9]+\\.[0-9]+", gen_decimal, "Decimal number"},
        {"[a-z]+[0-9]+", "[a-z]+[0-9]+", gen_letters_then_digits, "Letters then digits"},

        // Dot patterns (tests . metacharacter)
        {".*x", ".*x", gen_dot_star_x, "Dot star ending with x"},

        // Binary (2-char range)
        {"[01]+", "[01]+", gen_binary, "Binary digits"},

        // Printable ASCII
        {"[\\x20-\\x7e]+", "[\\x20-\\x7e]+", gen_printable, "Printable ASCII"},

        // ============ REAL-WORLD PATTERNS ============

        // Network: IPv4-like addresses
        {"IPv4-like", "[0-9.]+", gen_ipv4, "IPv4-like address sequences"},

        // Data: UUID-like patterns (escaped -)
        {"UUID-like", "[0-9a-f\\-]+", gen_uuid, "UUID-like hex with dashes"},

        // Logs: Timestamp patterns (escaped -)
        {"Timestamp", "[0-9:\\- ]+", gen_log_timestamp, "Log timestamp format"},

        // Web: Email-like patterns (escaped -)
        {"Email-like", "[a-zA-Z0-9._%+\\-]+@[a-zA-Z0-9.\\-]+", gen_email, "Email-like addresses"},

        // Web: HTTP header format (escaped -)
        {"HTTP-Header", "[A-Za-z\\-]+: [^\\r\\n]+", gen_http_header, "HTTP header lines"},

        // JSON: Identifier pattern
        {"JSON-key", "[a-zA-Z_][a-zA-Z0-9_]*", gen_json_key, "JSON/JS identifier"},

        // HTML: Simple tag pattern
        {"HTML-tag", "<[a-zA-Z][a-zA-Z0-9]*[^>]*>", gen_html_tag, "HTML tag structure"},
    };
}

// Get pattern by name
inline const PatternDef* get_pattern(const std::string& name) {
    static auto patterns = get_all_patterns();
    for (const auto& p : patterns) {
        if (p.name == name)
            return &p;
    }
    return nullptr;
}


// ============================================================================
// FALLBACK PATTERN GENERATORS (patterns that don't use SIMD)
// ============================================================================

// Repeated chars for backreference testing (.)\1+
// Generates strings like "aaaa", "bbbb" (50% match) or "abcd" (50% non-match)
inline std::vector<std::string> gen_repeated_char(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> l(0, 25);

    for (int i = 0; i < count; i++) {
        std::string s(len, 'a');
        if (i % 2 == 0) {
            // Match: all same character
            char c = 'a' + l(rng);
            for (size_t j = 0; j < len; j++)
                s[j] = c;
        } else {
            // Non-match: different characters
            for (size_t j = 0; j < len; j++)
                s[j] = 'a' + ((l(rng) + j) % 26);
        }
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// For lazy quantifiers: strings ending with 'x'
inline std::vector<std::string> gen_lazy_match(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> l(0, 24);  // a-y, no x
    std::uniform_int_distribution<size_t> pos(0, len > 1 ? len - 1 : 0);

    for (int i = 0; i < count; i++) {
        std::string s(len, 'a');
        for (size_t j = 0; j < len; j++)
            s[j] = 'a' + l(rng);  // no 'x'
        if (i % 2 == 0 && len > 0) {
            // Match: place 'x' somewhere
            s[pos(rng)] = 'x';
        }
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// For lookahead: alternating letter-digit
inline std::vector<std::string> gen_lookahead(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> l(0, 25);
    std::uniform_int_distribution<int> d(0, 9);

    for (int i = 0; i < count; i++) {
        std::string s(len, 'a');
        if (i % 2 == 0) {
            // Match: letter followed by digit
            for (size_t j = 0; j < len; j++)
                s[j] = (j % 2 == 0) ? ('a' + l(rng)) : ('0' + d(rng));
        } else {
            // Non-match: all letters
            for (size_t j = 0; j < len; j++)
                s[j] = 'a' + l(rng);
        }
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// For (abc)+ repeated group
inline std::vector<std::string> gen_repeated_group(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> l(0, 25);

    for (int i = 0; i < count; i++) {
        std::string s;
        if (i % 2 == 0) {
            // Match: repeat "abc"
            while (s.size() < len) s += "abc";
            s.resize(len);
        } else {
            // Non-match: random
            s.resize(len);
            for (size_t j = 0; j < len; j++)
                s[j] = 'a' + l(rng);
        }
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// ============================================================================
// FUSION SEGMENT COMPLEXITY - Fixed-size patterns for fusion benchmarking
// These generate inputs at NATURAL sizes for each pattern (ignoring len param)
// ============================================================================

// 1 segment: [0-9]{1,3} - generates "123" padded to len with more digits
// Uses len parameter to allow testing at different sizes
inline std::vector<std::string> gen_fusion_1seg(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> d(0, 9);

    for (int i = 0; i < count; i++) {
        std::string s;
        // Generate digits to fill len
        for (size_t j = 0; j < len; j++)
            s += ('0' + d(rng));

        // 50% invalid: add a letter
        if (i % 2 == 1) {
            s[len/2] = 'x';
        }
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// 2 segments: [0-9]{1,3}\.[0-9]{1,3} - generates "123.45" (~5-7 chars)
inline std::vector<std::string> gen_fusion_2seg(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> d(0, 9);
    std::uniform_int_distribution<int> digits(1, 3);

    for (int i = 0; i < count; i++) {
        std::string s;
        // First segment
        int n1 = digits(rng);
        for (int j = 0; j < n1; j++) s += ('0' + d(rng));
        s += '.';
        // Second segment
        int n2 = digits(rng);
        for (int j = 0; j < n2; j++) s += ('0' + d(rng));

        // 50% invalid: replace dot
        if (i % 2 == 1) {
            size_t dot = s.find('.');
            if (dot != std::string::npos) s[dot] = 'x';
        }
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// 4 segments: [0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3} (IPv4) - ~7-15 chars
inline std::vector<std::string> gen_fusion_4seg(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> octet(0, 255);

    for (int i = 0; i < count; i++) {
        std::string s = std::to_string(octet(rng)) + "." +
                       std::to_string(octet(rng)) + "." +
                       std::to_string(octet(rng)) + "." +
                       std::to_string(octet(rng));

        // 50% invalid: replace a dot
        if (i % 2 == 1) {
            size_t dot = s.find('.');
            if (dot != std::string::npos) s[dot] = 'x';
        }
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// 6 segments: [0-9a-f]{2}:[0-9a-f]{2}:... (MAC) - exactly 17 chars
inline std::vector<std::string> gen_fusion_6seg(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    const char hex[] = "0123456789abcdef";
    std::uniform_int_distribution<int> h(0, 15);

    for (int i = 0; i < count; i++) {
        std::string s;
        for (int seg = 0; seg < 6; seg++) {
            if (seg > 0) s += ':';
            s += hex[h(rng)];
            s += hex[h(rng)];
        }

        // 50% invalid: replace colon with 'g' (invalid hex)
        if (i % 2 == 1) {
            size_t colon = s.find(':');
            if (colon != std::string::npos) s[colon] = 'g';
        }
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// 8 segments: date-time like YYYY-MM-DD HH:MM:SS - exactly 19 chars
inline std::vector<std::string> gen_fusion_8seg(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> year(2000, 2025);
    std::uniform_int_distribution<int> month(1, 12);
    std::uniform_int_distribution<int> day(1, 28);
    std::uniform_int_distribution<int> hour(0, 23);
    std::uniform_int_distribution<int> minsec(0, 59);

    for (int i = 0; i < count; i++) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
                 year(rng), month(rng), day(rng),
                 hour(rng), minsec(rng), minsec(rng));
        std::string s(buf);

        // 50% invalid: replace separator
        if (i % 2 == 1) {
            s[4] = 'x';  // Replace first dash
        }
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// Long hex string - 64 bytes (above 48-byte SIMD threshold)
inline std::vector<std::string> gen_fusion_hex64(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    const char hex[] = "0123456789abcdef";
    std::uniform_int_distribution<int> h(0, 15);

    for (int i = 0; i < count; i++) {
        std::string s;
        for (int j = 0; j < 64; j++)
            s += hex[h(rng)];
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// Very long hex string - 128 bytes
inline std::vector<std::string> gen_fusion_hex128(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    const char hex[] = "0123456789abcdef";
    std::uniform_int_distribution<int> h(0, 15);

    for (int i = 0; i < count; i++) {
        std::string s;
        for (int j = 0; j < 128; j++)
            s += hex[h(rng)];
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// Long digit string - 64 bytes
inline std::vector<std::string> gen_fusion_digits64(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> d(0, 9);

    for (int i = 0; i < count; i++) {
        std::string s;
        for (int j = 0; j < 64; j++)
            s += ('0' + d(rng));
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// ============================================================================
// ADVERSARIAL PATTERNS - Not favorable for SIMD
// ============================================================================

// Fixed literal "test" - 100% matching
inline std::vector<std::string> gen_literal_test(size_t len, int count, unsigned int /*seed*/) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    for (int i = 0; i < count; i++) {
        // Must be exactly "test" for full match
        if (len >= 4) {
            inputs.push_back("test");
        } else {
            inputs.push_back(std::string("test").substr(0, len));
        }
    }
    return inputs;
}

// Fixed literal "hello world" - 100% matching
inline std::vector<std::string> gen_literal_hello_world(size_t len, int count, unsigned int /*seed*/) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    const std::string lit = "hello world";
    for (int i = 0; i < count; i++) {
        if (len >= lit.size()) {
            inputs.push_back(lit);
        } else {
            inputs.push_back(lit.substr(0, len));
        }
    }
    return inputs;
}

// Single character "a" - 100% matching
inline std::vector<std::string> gen_single_a_only(size_t len, int count, unsigned int /*seed*/) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    for (int i = 0; i < count; i++) {
        // Pattern is just "a", so input must be exactly "a"
        inputs.push_back("a");
    }
    (void)len; // Unused - pattern is fixed length
    return inputs;
}

// Bounded [a-z]{2,4} - 100% matching (2-4 letters)
inline std::vector<std::string> gen_bounded_short(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> letter(0, 25);
    std::uniform_int_distribution<size_t> bound_len(2, 4);

    for (int i = 0; i < count; i++) {
        size_t actual_len = std::min(bound_len(rng), len);
        actual_len = std::max(actual_len, size_t(2)); // At least 2
        std::string s;
        for (size_t j = 0; j < actual_len; j++) {
            s += 'a' + letter(rng);
        }
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// "id:[0-9]+" - 100% matching
inline std::vector<std::string> gen_prefix_digits(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> digit(0, 9);

    for (int i = 0; i < count; i++) {
        std::string s = "id:";
        size_t digits_needed = (len > 3) ? (len - 3) : 1;
        for (size_t j = 0; j < digits_needed; j++) {
            s += '0' + digit(rng);
        }
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// (cat|dog|bird|fish) - 100% matching one of the words
inline std::vector<std::string> gen_word_choice(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    const char* words[] = {"cat", "dog", "bird", "fish"};
    std::uniform_int_distribution<int> w(0, 3);

    for (int i = 0; i < count; i++) {
        inputs.push_back(words[w(rng)]);
    }
    return inputs;
}

// (www\.)?example - 100% matching
inline std::vector<std::string> gen_optional_www(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> choice(0, 1);

    for (int i = 0; i < count; i++) {
        if (choice(rng) == 0) {
            inputs.push_back("example");
        } else {
            inputs.push_back("www.example");
        }
    }
    return inputs;
}

// .*middle.* - 100% matching (contains "middle")
inline std::vector<std::string> gen_contains_middle(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> letter(0, 25);

    const std::string target = "middle";
    for (int i = 0; i < count; i++) {
        std::string s;
        // Add some prefix
        size_t prefix_len = (len > target.size() + 2) ? (len - target.size()) / 2 : 0;
        for (size_t j = 0; j < prefix_len; j++) {
            s += 'a' + letter(rng);
        }
        s += target;
        // Add suffix to reach len
        while (s.length() < len) {
            s += 'a' + letter(rng);
        }
        inputs.push_back(s.substr(0, std::max(len, target.size())));
    }
    return inputs;
}

// === LONGER ADVERSARIAL PATTERNS ===

// Long literal: 32 char string - 100% matching
inline std::vector<std::string> gen_literal_32(size_t /*len*/, int count, unsigned int /*seed*/) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    const std::string lit = "abcdefghijklmnopqrstuvwxyz012345";
    for (int i = 0; i < count; i++) {
        inputs.push_back(lit);
    }
    return inputs;
}

// Interleaved: a.b.c.d.e.f.g.h - 100% matching
inline std::vector<std::string> gen_interleaved(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> any(32, 126);  // printable ASCII
    
    for (int i = 0; i < count; i++) {
        // Pattern is a.b.c.d.e.f.g.h where . is any char
        std::string s;
        s += 'a'; s += static_cast<char>(any(rng));
        s += 'b'; s += static_cast<char>(any(rng));
        s += 'c'; s += static_cast<char>(any(rng));
        s += 'd'; s += static_cast<char>(any(rng));
        s += 'e'; s += static_cast<char>(any(rng));
        s += 'f'; s += static_cast<char>(any(rng));
        s += 'g'; s += static_cast<char>(any(rng));
        s += 'h';
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// Greek letters alternation - 100% matching
inline std::vector<std::string> gen_greek_word(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    const char* words[] = {"alpha", "beta", "gamma", "delta", "epsilon", 
                           "zeta", "eta", "theta", "iota", "kappa"};
    std::uniform_int_distribution<int> w(0, 9);
    
    for (int i = 0; i < count; i++) {
        inputs.push_back(words[w(rng)]);
    }
    return inputs;
}

// Counted repetition a{20} - 100% matching (exactly 20 'a's)
inline std::vector<std::string> gen_a_20(size_t /*len*/, int count, unsigned int /*seed*/) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    for (int i = 0; i < count; i++) {
        inputs.push_back(std::string(20, 'a'));
    }
    return inputs;
}

// Nested optional: (a(b(c)?)?)?d - 100% matching
inline std::vector<std::string> gen_nested_optional(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> choice(0, 3);
    
    for (int i = 0; i < count; i++) {
        switch (choice(rng)) {
            case 0: inputs.push_back("d"); break;           // just d
            case 1: inputs.push_back("ad"); break;          // a + d
            case 2: inputs.push_back("abd"); break;         // a + b + d
            case 3: inputs.push_back("abcd"); break;        // a + b + c + d
        }
    }
    return inputs;
}

// Prefix with alternation suffix: data_(one|two|three|four|five) - 100% matching
inline std::vector<std::string> gen_data_suffix(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    const char* suffixes[] = {"one", "two", "three", "four", "five"};
    std::uniform_int_distribution<int> s(0, 4);
    
    for (int i = 0; i < count; i++) {
        inputs.push_back(std::string("data_") + suffixes[s(rng)]);
    }
    return inputs;
}

// === WORST CASE GENERATORS ===

// a? - optional single char: "" or "a"
inline std::vector<std::string> gen_optional_a(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> choice(0, 1);
    
    for (int i = 0; i < count; i++) {
        inputs.push_back(choice(rng) ? "a" : "");
    }
    return inputs;
}

// a?b?c?d? - 0 to 4 chars
inline std::vector<std::string> gen_optional_4(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> bits(0, 15);
    
    for (int i = 0; i < count; i++) {
        std::string s;
        int b = bits(rng);
        if (b & 1) s += 'a';
        if (b & 2) s += 'b';
        if (b & 4) s += 'c';
        if (b & 8) s += 'd';
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// Single letter a-z
inline std::vector<std::string> gen_single_letter(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> letter(0, 25);
    
    for (int i = 0; i < count; i++) {
        inputs.push_back(std::string(1, 'a' + letter(rng)));
    }
    return inputs;
}

// Empty or "x" for x* pattern
inline std::vector<std::string> gen_empty_or_x(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> choice(0, 2);
    
    for (int i = 0; i < count; i++) {
        int c = choice(rng);
        if (c == 0) inputs.push_back("");
        else if (c == 1) inputs.push_back("x");
        else inputs.push_back("xx");
    }
    return inputs;
}

// "ab" only
inline std::vector<std::string> gen_ab_only(size_t /*len*/, int count, unsigned int /*seed*/) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    for (int i = 0; i < count; i++) {
        inputs.push_back("ab");
    }
    return inputs;
}

// "abc" only
inline std::vector<std::string> gen_abc_only(size_t /*len*/, int count, unsigned int /*seed*/) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    for (int i = 0; i < count; i++) {
        inputs.push_back("abc");
    }
    return inputs;
}

// === EVEN MORE EXTREME GENERATORS ===

// "a" or "b" for a|b
inline std::vector<std::string> gen_a_or_b(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> choice(0, 1);
    for (int i = 0; i < count; i++) {
        inputs.push_back(choice(rng) ? "a" : "b");
    }
    return inputs;
}

// Just "." for escaped dot
inline std::vector<std::string> gen_dot_only(size_t /*len*/, int count, unsigned int /*seed*/) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    for (int i = 0; i < count; i++) {
        inputs.push_back(".");
    }
    return inputs;
}

// "", "a", "b", "ab" for a?b?
inline std::vector<std::string> gen_optional_ab(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> choice(0, 3);
    for (int i = 0; i < count; i++) {
        switch(choice(rng)) {
            case 0: inputs.push_back(""); break;
            case 1: inputs.push_back("a"); break;
            case 2: inputs.push_back("b"); break;
            case 3: inputs.push_back("ab"); break;
        }
    }
    return inputs;
}

// Short "a", "aa", "aaa" for a+
inline std::vector<std::string> gen_short_a(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> len_dist(1, 4);
    for (int i = 0; i < count; i++) {
        inputs.push_back(std::string(len_dist(rng), 'a'));
    }
    return inputs;
}

// "a.b" for a\.b
inline std::vector<std::string> gen_a_dot_b(size_t /*len*/, int count, unsigned int /*seed*/) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    for (int i = 0; i < count; i++) {
        inputs.push_back("a.b");
    }
    return inputs;
}

// "a" or "ab" for ab?
inline std::vector<std::string> gen_a_or_ab(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> choice(0, 1);
    for (int i = 0; i < count; i++) {
        inputs.push_back(choice(rng) ? "a" : "ab");
    }
    return inputs;
}

// "a" or "" for a|
inline std::vector<std::string> gen_a_or_empty(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> choice(0, 1);
    for (int i = 0; i < count; i++) {
        inputs.push_back(choice(rng) ? "a" : "");
    }
    return inputs;
}

// === ROUND 3 GENERATORS ===

// Any single char for "."
inline std::vector<std::string> gen_any_single(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> c(32, 126);
    for (int i = 0; i < count; i++) {
        inputs.push_back(std::string(1, static_cast<char>(c(rng))));
    }
    return inputs;
}

// Any two chars for ".."
inline std::vector<std::string> gen_any_two(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> c(32, 126);
    for (int i = 0; i < count; i++) {
        std::string s;
        s += static_cast<char>(c(rng));
        s += static_cast<char>(c(rng));
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// "abcd" only
inline std::vector<std::string> gen_abcd_only(size_t /*len*/, int count, unsigned int /*seed*/) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    for (int i = 0; i < count; i++) inputs.push_back("abcd");
    return inputs;
}

// "aa" only for a{2}
inline std::vector<std::string> gen_aa_only(size_t /*len*/, int count, unsigned int /*seed*/) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    for (int i = 0; i < count; i++) inputs.push_back("aa");
    return inputs;
}

// "a" or "aa" for a{1,2}
inline std::vector<std::string> gen_a_or_aa(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> choice(0, 1);
    for (int i = 0; i < count; i++) {
        inputs.push_back(choice(rng) ? "a" : "aa");
    }
    return inputs;
}

// "ac" or "abc" for ab?c
inline std::vector<std::string> gen_ac_or_abc(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> choice(0, 1);
    for (int i = 0; i < count; i++) {
        inputs.push_back(choice(rng) ? "ac" : "abc");
    }
    return inputs;
}

// "axbxc" for a.b.c (any char between)
inline std::vector<std::string> gen_axbxc(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> c(32, 126);
    for (int i = 0; i < count; i++) {
        std::string s = "a";
        s += static_cast<char>(c(rng));
        s += "b";
        s += static_cast<char>(c(rng));
        s += "c";
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// Ends with 'a' for .*a
inline std::vector<std::string> gen_ends_a(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> c(98, 122); // b-z
    for (int i = 0; i < count; i++) {
        std::string s;
        for (size_t j = 0; j + 1 < len; j++) {
            s += static_cast<char>(c(rng));
        }
        s += 'a';
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// "ab" or "cd" for ab|cd
inline std::vector<std::string> gen_ab_or_cd(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> choice(0, 1);
    for (int i = 0; i < count; i++) {
        inputs.push_back(choice(rng) ? "ab" : "cd");
    }
    return inputs;
}

// ac, ad, bc, bd for (a|b)(c|d)
inline std::vector<std::string> gen_ac_ad_bc_bd(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> choice(0, 3);
    const char* opts[] = {"ac", "ad", "bc", "bd"};
    for (int i = 0; i < count; i++) {
        inputs.push_back(opts[choice(rng)]);
    }
    return inputs;
}

// "hello" only
inline std::vector<std::string> gen_hello(size_t /*len*/, int count, unsigned int /*seed*/) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    for (int i = 0; i < count; i++) inputs.push_back("hello");
    return inputs;
}

// === ROUND 4 GENERATORS ===

inline std::vector<std::string> gen_foobar(size_t /*len*/, int count, unsigned int /*seed*/) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    for (int i = 0; i < count; i++) inputs.push_back("foobar");
    return inputs;
}

inline std::vector<std::string> gen_testing(size_t /*len*/, int count, unsigned int /*seed*/) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    for (int i = 0; i < count; i++) inputs.push_back("testing");
    return inputs;
}

inline std::vector<std::string> gen_8char(size_t /*len*/, int count, unsigned int /*seed*/) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    for (int i = 0; i < count; i++) inputs.push_back("abcdefgh");
    return inputs;
}

inline std::vector<std::string> gen_15char(size_t /*len*/, int count, unsigned int /*seed*/) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    for (int i = 0; i < count; i++) inputs.push_back("abcdefghijklmno");
    return inputs;
}

inline std::vector<std::string> gen_aab(size_t /*len*/, int count, unsigned int /*seed*/) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    for (int i = 0; i < count; i++) inputs.push_back("aab");
    return inputs;
}

// "y", "xy", "xxy" for x*y
inline std::vector<std::string> gen_xy(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> choice(0, 2);
    for (int i = 0; i < count; i++) {
        switch(choice(rng)) {
            case 0: inputs.push_back("y"); break;
            case 1: inputs.push_back("xy"); break;
            case 2: inputs.push_back("xxy"); break;
        }
    }
    return inputs;
}

// === ROUND 5 GENERATORS ===

inline std::vector<std::string> gen_16char(size_t /*len*/, int count, unsigned int /*seed*/) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    for (int i = 0; i < count; i++) inputs.push_back("abcdefghijklmnop");
    return inputs;
}

inline std::vector<std::string> gen_20char(size_t /*len*/, int count, unsigned int /*seed*/) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    for (int i = 0; i < count; i++) inputs.push_back("abcdefghijklmnopqrst");
    return inputs;
}

inline std::vector<std::string> gen_24char(size_t /*len*/, int count, unsigned int /*seed*/) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    for (int i = 0; i < count; i++) inputs.push_back("abcdefghijklmnopqrstuvwx");
    return inputs;
}

inline std::vector<std::string> gen_28char(size_t /*len*/, int count, unsigned int /*seed*/) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    for (int i = 0; i < count; i++) inputs.push_back("abcdefghijklmnopqrstuvwxyzab");
    return inputs;
}

inline std::vector<std::string> gen_greek(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> choice(0, 7);
    const char* words[] = {"alpha", "beta", "gamma", "delta", "epsilon", "zeta", "eta", "theta"};
    for (int i = 0; i < count; i++) inputs.push_back(words[choice(rng)]);
    return inputs;
}

inline std::vector<std::string> gen_optional_6(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> bits(0, 63);
    for (int i = 0; i < count; i++) {
        std::string s;
        int b = bits(rng);
        if (b & 1) s += 'a';
        if (b & 2) s += 'b';
        if (b & 4) s += 'c';
        if (b & 8) s += 'd';
        if (b & 16) s += 'e';
        if (b & 32) s += 'f';
        inputs.push_back(std::move(s));
    }
    return inputs;
}

inline std::vector<std::string> gen_a10(size_t /*len*/, int count, unsigned int /*seed*/) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    for (int i = 0; i < count; i++) inputs.push_back("aaaaaaaaaa");
    return inputs;
}

inline std::vector<std::string> gen_a50(size_t /*len*/, int count, unsigned int /*seed*/) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    for (int i = 0; i < count; i++) inputs.push_back(std::string(50, 'a'));
    return inputs;
}

inline std::vector<std::string> gen_abcde(size_t /*len*/, int count, unsigned int /*seed*/) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    for (int i = 0; i < count; i++) inputs.push_back("abcde");
    return inputs;
}

inline std::vector<std::string> gen_pairs(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> choice(0, 5);
    const char* pairs[] = {"aa", "bb", "cc", "dd", "ee", "ff"};
    for (int i = 0; i < count; i++) inputs.push_back(pairs[choice(rng)]);
    return inputs;
}

inline std::vector<std::string> gen_foo_xxx_bar(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> c('a', 'z');
    for (int i = 0; i < count; i++) {
        std::string s = "foo";
        s += static_cast<char>(c(rng));
        s += static_cast<char>(c(rng));
        s += static_cast<char>(c(rng));
        s += "bar";
        inputs.push_back(std::move(s));
    }
    return inputs;
}

inline std::vector<std::string> gen_dot_sep_6(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> c(32, 126);
    for (int i = 0; i < count; i++) {
        std::string s = "a";
        s += static_cast<char>(c(rng));
        s += "b";
        s += static_cast<char>(c(rng));
        s += "c";
        s += static_cast<char>(c(rng));
        s += "d";
        s += static_cast<char>(c(rng));
        s += "e";
        s += static_cast<char>(c(rng));
        s += "f";
        inputs.push_back(std::move(s));
    }
    return inputs;
}

inline std::vector<std::string> gen_4x3(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> choice(0, 3);
    const char* words[] = {"foo", "bar", "baz", "qux"};
    for (int i = 0; i < count; i++) inputs.push_back(words[choice(rng)]);
    return inputs;
}

// === ROUND 6 GENERATORS ===

inline std::vector<std::string> gen_64char(size_t /*len*/, int count, unsigned int /*seed*/) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    for (int i = 0; i < count; i++) 
        inputs.push_back("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789ab");
    return inputs;
}

inline std::vector<std::string> gen_a100(size_t /*len*/, int count, unsigned int /*seed*/) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    for (int i = 0; i < count; i++) inputs.push_back(std::string(100, 'a'));
    return inputs;
}

inline std::vector<std::string> gen_a200(size_t /*len*/, int count, unsigned int /*seed*/) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    for (int i = 0; i < count; i++) inputs.push_back(std::string(200, 'a'));
    return inputs;
}

inline std::vector<std::string> gen_numbers(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> choice(0, 11);
    const char* words[] = {"one", "two", "three", "four", "five", "six", 
                           "seven", "eight", "nine", "ten", "eleven", "twelve"};
    for (int i = 0; i < count; i++) inputs.push_back(words[choice(rng)]);
    return inputs;
}

inline std::vector<std::string> gen_a1a1a(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> letter('a', 'z');
    std::uniform_int_distribution<int> digit('0', '9');
    for (int i = 0; i < count; i++) {
        std::string s;
        s += static_cast<char>(letter(rng));
        s += static_cast<char>(digit(rng));
        s += static_cast<char>(letter(rng));
        s += static_cast<char>(digit(rng));
        s += static_cast<char>(letter(rng));
        inputs.push_back(std::move(s));
    }
    return inputs;
}

inline std::vector<std::string> gen_optional_8(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> bits(0, 255);
    for (int i = 0; i < count; i++) {
        std::string s;
        int b = bits(rng);
        if (b & 1) s += 'a';
        if (b & 2) s += 'b';
        if (b & 4) s += 'c';
        if (b & 8) s += 'd';
        if (b & 16) s += 'e';
        if (b & 32) s += 'f';
        if (b & 64) s += 'g';
        if (b & 128) s += 'h';
        inputs.push_back(std::move(s));
    }
    return inputs;
}

inline std::vector<std::string> gen_id_name(size_t len, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> digit('0', '9');
    std::uniform_int_distribution<int> letter('a', 'z');
    std::uniform_int_distribution<size_t> num_len(1, std::max(size_t(1), len/4));
    for (int i = 0; i < count; i++) {
        std::string s = "id_";
        size_t n = num_len(rng);
        for (size_t j = 0; j < n; j++) s += static_cast<char>(digit(rng));
        s += "_name_";
        for (size_t j = 0; j < n; j++) s += static_cast<char>(letter(rng));
        inputs.push_back(std::move(s));
    }
    return inputs;
}

inline std::vector<std::string> gen_10_pairs(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> choice(0, 9);
    const char* pairs[] = {"aa", "bb", "cc", "dd", "ee", "ff", "gg", "hh", "ii", "jj"};
    for (int i = 0; i < count; i++) inputs.push_back(pairs[choice(rng)]);
    return inputs;
}

inline std::vector<std::string> gen_dot_sep_10(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> c(32, 126);
    for (int i = 0; i < count; i++) {
        std::string s;
        for (char ch = 'a'; ch <= 'j'; ch++) {
            if (ch != 'a') s += static_cast<char>(c(rng));
            s += ch;
        }
        inputs.push_back(std::move(s));
    }
    return inputs;
}

inline std::vector<std::string> gen_start_range_end(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> c(0, 61);
    for (int i = 0; i < count; i++) {
        std::string s = "start";
        for (int j = 0; j < 10; j++) {
            int v = c(rng);
            if (v < 26) s += static_cast<char>('a' + v);
            else if (v < 52) s += static_cast<char>('A' + v - 26);
            else s += static_cast<char>('0' + v - 52);
        }
        s += "end";
        inputs.push_back(std::move(s));
    }
    return inputs;
}

inline std::vector<std::string> gen_alt_16(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> choice(0, 15);
    for (int i = 0; i < count; i++) {
        inputs.push_back(std::string(1, 'a' + choice(rng)));
    }
    return inputs;
}

inline std::vector<std::string> gen_class_555(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> letter('a', 'z');
    std::uniform_int_distribution<int> digit('0', '9');
    for (int i = 0; i < count; i++) {
        std::string s;
        for (int j = 0; j < 5; j++) s += static_cast<char>(letter(rng));
        for (int j = 0; j < 5; j++) s += static_cast<char>(digit(rng));
        for (int j = 0; j < 5; j++) s += static_cast<char>(letter(rng));
        inputs.push_back(std::move(s));
    }
    return inputs;
}

inline std::vector<std::string> gen_class_55(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> letter('a', 'z');
    std::uniform_int_distribution<int> digit('0', '9');
    for (int i = 0; i < count; i++) {
        std::string s;
        for (int j = 0; j < 5; j++) s += static_cast<char>(letter(rng));
        for (int j = 0; j < 5; j++) s += static_cast<char>(digit(rng));
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// === ROUND 7 GENERATORS ===

inline std::vector<std::string> gen_alt_seq_4(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> bit(0, 1);
    for (int i = 0; i < count; i++) {
        std::string s;
        s += bit(rng) ? 'a' : 'b';
        s += bit(rng) ? 'c' : 'd';
        s += bit(rng) ? 'e' : 'f';
        s += bit(rng) ? 'g' : 'h';
        inputs.push_back(std::move(s));
    }
    return inputs;
}

inline std::vector<std::string> gen_dot_chain_12(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> c(32, 126);
    for (int i = 0; i < count; i++) {
        std::string s;
        for (char ch = 'a'; ch <= 'l'; ch++) {
            if (ch != 'a') s += static_cast<char>(c(rng));
            s += ch;
        }
        inputs.push_back(std::move(s));
    }
    return inputs;
}

inline std::vector<std::string> gen_abcdefgh(size_t /*len*/, int count, unsigned int /*seed*/) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    for (int i = 0; i < count; i++) inputs.push_back("abcdefgh");
    return inputs;
}

inline std::vector<std::string> gen_abcdef(size_t /*len*/, int count, unsigned int /*seed*/) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    for (int i = 0; i < count; i++) inputs.push_back("abcdef");
    return inputs;
}

inline std::vector<std::string> gen_varied_alt(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> choice(0, 4);
    const char* words[] = {"a", "bb", "ccc", "dddd", "eeeee"};
    for (int i = 0; i < count; i++) inputs.push_back(words[choice(rng)]);
    return inputs;
}

inline std::vector<std::string> gen_lit_dot_lit(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> c(32, 126);
    for (int i = 0; i < count; i++) {
        std::string s = "ab";
        s += static_cast<char>(c(rng));
        s += "cd";
        s += static_cast<char>(c(rng));
        s += "ef";
        s += static_cast<char>(c(rng));
        s += "gh";
        inputs.push_back(std::move(s));
    }
    return inputs;
}

inline std::vector<std::string> gen_5_words(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> choice(0, 4);
    const char* words[] = {"the", "quick", "brown", "fox", "jumps"};
    for (int i = 0; i < count; i++) inputs.push_back(words[choice(rng)]);
    return inputs;
}

inline std::vector<std::string> gen_optional_10(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> bits(0, 1023);
    for (int i = 0; i < count; i++) {
        std::string s;
        int b = bits(rng);
        for (int j = 0; j < 10; j++) {
            if (b & (1 << j)) s += static_cast<char>('a' + j);
        }
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// === 4-CHAR INVESTIGATION ===
inline std::vector<std::string> gen_wxyz(size_t /*len*/, int count, unsigned int /*seed*/) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    for (int i = 0; i < count; i++) inputs.push_back("wxyz");
    return inputs;
}

inline std::vector<std::string> gen_1234(size_t /*len*/, int count, unsigned int /*seed*/) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    for (int i = 0; i < count; i++) inputs.push_back("1234");
    return inputs;
}

inline std::vector<std::string> gen_best(size_t /*len*/, int count, unsigned int /*seed*/) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    for (int i = 0; i < count; i++) inputs.push_back("best");
    return inputs;
}

inline std::vector<std::string> gen_fest(size_t /*len*/, int count, unsigned int /*seed*/) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    for (int i = 0; i < count; i++) inputs.push_back("fest");
    return inputs;
}

inline std::vector<std::string> gen_rest(size_t /*len*/, int count, unsigned int /*seed*/) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    for (int i = 0; i < count; i++) inputs.push_back("rest");
    return inputs;
}

inline std::vector<std::string> gen_abab(size_t /*len*/, int count, unsigned int /*seed*/) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    for (int i = 0; i < count; i++) inputs.push_back("abab");
    return inputs;
}

inline std::vector<std::string> gen_aaaa(size_t /*len*/, int count, unsigned int /*seed*/) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    for (int i = 0; i < count; i++) inputs.push_back("aaaa");
    return inputs;
}

inline std::vector<std::string> gen_aabb(size_t /*len*/, int count, unsigned int /*seed*/) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    for (int i = 0; i < count; i++) inputs.push_back("aabb");
    return inputs;
}

// === MONSTER GENERATORS ===

inline std::vector<std::string> gen_128char(size_t /*len*/, int count, unsigned int /*seed*/) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    for (int i = 0; i < count; i++) 
        inputs.push_back("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcd");
    return inputs;
}

inline std::vector<std::string> gen_lower64(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> c('a', 'z');
    for (int i = 0; i < count; i++) {
        std::string s;
        for (int j = 0; j < 64; j++) s += static_cast<char>(c(rng));
        inputs.push_back(std::move(s));
    }
    return inputs;
}

inline std::vector<std::string> gen_digits100(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> c('0', '9');
    for (int i = 0; i < count; i++) {
        std::string s;
        for (int j = 0; j < 100; j++) s += static_cast<char>(c(rng));
        inputs.push_back(std::move(s));
    }
    return inputs;
}

inline std::vector<std::string> gen_alt_types(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> type(0, 2);
    std::uniform_int_distribution<int> lower('a', 'z');
    std::uniform_int_distribution<int> digit('0', '9');
    std::uniform_int_distribution<int> upper('A', 'Z');
    for (int i = 0; i < count; i++) {
        std::string s;
        int t = type(rng);
        for (int j = 0; j < 10; j++) {
            if (t == 0) s += static_cast<char>(lower(rng));
            else if (t == 1) s += static_cast<char>(digit(rng));
            else s += static_cast<char>(upper(rng));
        }
        inputs.push_back(std::move(s));
    }
    return inputs;
}

inline std::vector<std::string> gen_triple_range(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> lower('a', 'z');
    std::uniform_int_distribution<int> digit('0', '9');
    std::uniform_int_distribution<int> upper('A', 'Z');
    for (int i = 0; i < count; i++) {
        std::string s;
        for (int j = 0; j < 20; j++) s += static_cast<char>(lower(rng));
        for (int j = 0; j < 20; j++) s += static_cast<char>(digit(rng));
        for (int j = 0; j < 20; j++) s += static_cast<char>(upper(rng));
        inputs.push_back(std::move(s));
    }
    return inputs;
}

inline std::vector<std::string> gen_12x3(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> choice(0, 11);
    const char* opts[] = {"abc", "def", "ghi", "jkl", "mno", "pqr", 
                          "stu", "vwx", "yz0", "123", "456", "789"};
    for (int i = 0; i < count; i++) inputs.push_back(opts[choice(rng)]);
    return inputs;
}

inline std::vector<std::string> gen_alnum50(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> c(0, 61);
    for (int i = 0; i < count; i++) {
        std::string s;
        for (int j = 0; j < 50; j++) {
            int v = c(rng);
            if (v < 26) s += static_cast<char>('a' + v);
            else if (v < 52) s += static_cast<char>('A' + v - 26);
            else s += static_cast<char>('0' + v - 52);
        }
        inputs.push_back(std::move(s));
    }
    return inputs;
}

inline std::vector<std::string> gen_lit_range_lit_big(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> c('a', 'z');
    for (int i = 0; i < count; i++) {
        std::string s = "prefix_";
        for (int j = 0; j < 30; j++) s += static_cast<char>(c(rng));
        s += "_suffix";
        inputs.push_back(std::move(s));
    }
    return inputs;
}

// === TRULY MASSIVE GENERATORS ===

inline std::vector<std::string> gen_256char(size_t /*len*/, int count, unsigned int /*seed*/) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    for (int i = 0; i < count; i++) 
        inputs.push_back("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcd");
    return inputs;
}

inline std::vector<std::string> gen_lower256(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> c('a', 'z');
    for (int i = 0; i < count; i++) {
        std::string s;
        s.reserve(256);
        for (int j = 0; j < 256; j++) s += static_cast<char>(c(rng));
        inputs.push_back(std::move(s));
    }
    return inputs;
}

inline std::vector<std::string> gen_digits500(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> c('0', '9');
    for (int i = 0; i < count; i++) {
        std::string s;
        s.reserve(500);
        for (int j = 0; j < 500; j++) s += static_cast<char>(c(rng));
        inputs.push_back(std::move(s));
    }
    return inputs;
}

inline std::vector<std::string> gen_alpha1000(size_t /*len*/, int count, unsigned int seed) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> c(0, 51);
    for (int i = 0; i < count; i++) {
        std::string s;
        s.reserve(1000);
        for (int j = 0; j < 1000; j++) {
            int v = c(rng);
            if (v < 26) s += static_cast<char>('a' + v);
            else s += static_cast<char>('A' + v - 26);
        }
        inputs.push_back(std::move(s));
    }
    return inputs;
}

} // namespace bench
