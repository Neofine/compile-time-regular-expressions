#include <ctre.hpp>
#include <ctre/wrapper_with_prefilter.hpp>
#include <string>

int main() {
    std::string input(32, 'a');
    volatile bool result;
    for (int i = 0; i < 10000000; ++i) {
        result = ctre::match_with_prefilter<"a+">(input);
    }
    return result ? 0 : 1;
}
