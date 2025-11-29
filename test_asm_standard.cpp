#include <ctre.hpp>
#include <string>

bool test_match(const std::string& input) {
    return ctre::match<"a+">(input);
}
