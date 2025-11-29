#include <ctre.hpp>
#include <ctre/decomposition.hpp>  // Explicitly include
#include <iostream>
#include <chrono>
#include <string>

int main() {
    std::string input(32, 'a');
    volatile bool dummy = false;
    for (int i = 0; i < 1000000; ++i) {
        dummy = ctre::match<"a+">(input);
    }
    return 0;
}
