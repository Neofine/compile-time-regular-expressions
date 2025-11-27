#include <iostream>
#include <string>
#include <chrono>
#include <ctre.hpp>

// Benchmark helper
template<typename F>
double benchmark(F&& f, int warmup = 10000, int iterations = 100000) {
    for (int i = 0; i < warmup; ++i) f();

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        f();
    }
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::nano>(end - start).count() / iterations;
}

int main() {
    std::cout << "==========================================\n";
    std::cout << "Real-World Pattern Benchmarks\n";
    std::cout << "==========================================\n\n";

    // 1. Email-like username matching
    {
        std::string email = "username123";
        auto t = benchmark([&]() {
            return ctre::match<"[a-zA-Z][a-zA-Z0-9_]+">(email);
        });
        std::cout << "Username (email-style):     " << t << " ns\n";
    }

    // 2. URL protocol
    {
        std::string url = "https://example.com";
        auto t = benchmark([&]() {
            return ctre::search<"https?">(url);
        });
        std::cout << "URL protocol:               " << t << " ns\n";
    }

    // 3. IPv4 address
    {
        std::string ip = "192.168.1.1";
        auto t = benchmark([&]() {
            return ctre::search<"[0-9]{1,3}">(ip);
        });
        std::cout << "IPv4 octet:                 " << t << " ns\n";
    }

    // 4. Log timestamp
    {
        std::string timestamp = "2025-11-27 14:30:45";
        auto t = benchmark([&]() {
            return ctre::match<"[0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2}">(timestamp);
        });
        std::cout << "Log timestamp:              " << t << " ns\n";
    }

    // 5. Hexadecimal color
    {
        std::string color = "#FF5733";
        auto t = benchmark([&]() {
            return ctre::match<"#[0-9a-fA-F]{6}">(color);
        });
        std::cout << "Hex color:                  " << t << " ns\n";
    }

    // 6. Log level extraction
    {
        std::string log = "[2025-11-27 14:30:45] ERROR: Connection timeout";
        auto t = benchmark([&]() {
            return ctre::search<"ERROR|WARN|INFO|DEBUG">(log);
        });
        std::cout << "Log level extraction:       " << t << " ns\n";
    }

    // 7. JSON key matching
    {
        std::string json_key = "user_id";
        auto t = benchmark([&]() {
            return ctre::match<"[a-zA-Z_][a-zA-Z0-9_]*">(json_key);
        });
        std::cout << "JSON key identifier:        " << t << " ns\n";
    }

    // 8. Phone number digits
    {
        std::string phone = "5551234567";
        auto t = benchmark([&]() {
            return ctre::match<"[0-9]{10}">(phone);
        });
        std::cout << "Phone number (digits):      " << t << " ns\n";
    }

    // 9. Alphanumeric with dash
    {
        std::string slug = "my-article-slug-123";
        auto t = benchmark([&]() {
            return ctre::match<"[a-z0-9\\-]+">(slug);
        });
        std::cout << "URL slug:                   " << t << " ns\n";
    }

    // 10. Credit card-like number
    {
        std::string cc = "4532123456789010";
        auto t = benchmark([&]() {
            return ctre::match<"[0-9]{16}">(cc);
        });
        std::cout << "16-digit number:            " << t << " ns\n";
    }

    // 11. UUID (hex segments)
    {
        std::string uuid = "550e8400-e29b-41d4-a716-446655440000";
        auto t = benchmark([&]() {
            return ctre::match<"[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}">(uuid);
        });
        std::cout << "UUID:                       " << t << " ns\n";
    }

    // 12. HTTP header name
    {
        std::string header = "Content-Type";
        auto t = benchmark([&]() {
            return ctre::match<"[A-Z][a-zA-Z\\-]+">(header);
        });
        std::cout << "HTTP header name:           " << t << " ns\n";
    }

    // 13. HTML tag name
    {
        std::string tag = "<div>";
        auto t = benchmark([&]() {
            return ctre::search<"<[a-zA-Z][a-zA-Z0-9]*">(tag);
        });
        std::cout << "HTML tag name:              " << t << " ns\n";
    }

    // 14. Version number
    {
        std::string version = "v1.2.3";
        auto t = benchmark([&]() {
            return ctre::match<"v?[0-9]+\\.[0-9]+\\.[0-9]+">(version);
        });
        std::cout << "Version number:             " << t << " ns\n";
    }

    // 15. Domain name
    {
        std::string domain = "example.com";
        auto t = benchmark([&]() {
            return ctre::match<"[a-zA-Z0-9\\-]+\\.[a-zA-Z]+">(domain);
        });
        std::cout << "Domain name:                " << t << " ns\n";
    }

    // 16. File extension
    {
        std::string filename = "document.pdf";
        auto t = benchmark([&]() {
            return ctre::search<"\\.[a-zA-Z0-9]+">(filename);
        });
        std::cout << "File extension:             " << t << " ns\n";
    }

    // 17. Repeated word
    {
        std::string text = "hello hello world";
        auto t = benchmark([&]() {
            return ctre::search<"([a-z]+) \\1">(text);
        });
        std::cout << "Repeated word:              " << t << " ns\n";
    }

    // 18. Alphanumeric password
    {
        std::string password = "MyP@ssw0rd";
        auto t = benchmark([&]() {
            return ctre::match<"[a-zA-Z0-9@#$%^&*]+">(password);
        });
        std::cout << "Password validation:        " << t << " ns\n";
    }

    std::cout << "\n==========================================\n";
    std::cout << "Real-world patterns tested: 18\n";
    std::cout << "All patterns represent common use cases\n";
    std::cout << "==========================================\n";

    return 0;
}
