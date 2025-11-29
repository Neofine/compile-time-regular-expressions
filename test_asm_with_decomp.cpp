#include <ctre.hpp>
#include <ctre/decomposition.hpp>  // Include analysis
#include <string>

bool test_match(const std::string& input) {
    return ctre::match<"a+">(input);  // Same code
}
