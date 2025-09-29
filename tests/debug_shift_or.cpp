#include <ctre.hpp>
#include <iostream>
#include <string>

int main() {
    // Test if CTRE can actually match simple patterns
    std::string test1 = "Hello World";
    std::string test2 = "ABCDEFGH";
    std::string test3 = "CTRE is awesome";

    std::cout << "Testing CTRE string matching:\n";
    std::cout << "============================\n";

    // Test simple patterns
    std::cout << "Test string: '" << test1 << "'\n";
    std::cout << "Match 'Hello': " << (ctre::match<"Hello">(test1) ? "YES" : "NO") << "\n";
    std::cout << "Match 'World': " << (ctre::match<"World">(test1) ? "YES" : "NO") << "\n";
    std::cout << "Match 'XYZ': " << (ctre::match<"XYZ">(test1) ? "YES" : "NO") << "\n";

    std::cout << "\nTest string: '" << test2 << "'\n";
    std::cout << "Match 'ABCD': " << (ctre::match<"ABCD">(test2) ? "YES" : "NO") << "\n";
    std::cout << "Match 'EFGH': " << (ctre::match<"EFGH">(test2) ? "YES" : "NO") << "\n";
    std::cout << "Match 'ABCDEFGH': " << (ctre::match<"ABCDEFGH">(test2) ? "YES" : "NO") << "\n";

    std::cout << "\nTest string: '" << test3 << "'\n";
    std::cout << "Match 'CTRE': " << (ctre::match<"CTRE">(test3) ? "YES" : "NO") << "\n";
    std::cout << "Match 'awesome': " << (ctre::match<"awesome">(test3) ? "YES" : "NO") << "\n";

    // Test our generated data
    std::cout << "\nTesting generated data:\n";
    std::cout << "======================\n";

    // Generate test data like the benchmark does
    std::string generated = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    // Insert pattern at position 10
    generated[10] = 'C';
    generated[11] = 'T';
    generated[12] = 'R';
    generated[13] = 'E';

    std::cout << "Generated string: '" << generated << "'\n";
    std::cout << "Match 'CTRE': " << (ctre::match<"CTRE">(generated) ? "YES" : "NO") << "\n";

    return 0;
}
