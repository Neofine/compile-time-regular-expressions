#include <iostream>
#include <string>
#include <ctre.hpp>

int main() {
    std::cout << "Testing a* pattern to see what function is called" << std::endl;

    std::string text = "aaaa";
    auto match = ctre::match<"a*">(text);

    if (match) {
        std::cout << "a* pattern matched: " << match.to_string() << std::endl;
    } else {
        std::cout << "a* pattern did NOT match." << std::endl;
    }

    return 0;
}
