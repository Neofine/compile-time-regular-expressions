#include <ctre.hpp>
#include <iostream>
#include <typeinfo>
#include <cxxabi.h>

template<typename T>
std::string type_name() {
    int status;
    char* demangled = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, &status);
    std::string result = (status == 0) ? demangled : typeid(T).name();
    free(demangled);
    return result;
}

template <typename Pattern>
void print_unwrapped_type(const char* name) {
    using RawAST = ctre::decomposition::unwrap_regex_t<Pattern>;
    std::cout << "\n" << name << ":\n";
    std::cout << type_name<RawAST>() << "\n";

    constexpr size_t pos_count = ctre::glushkov::count_positions<RawAST>();
    std::cout << "Position count: " << pos_count << "\n";
}

int main() {
    std::cout << "=== Type Unwrapping Debug ===\n";

    print_unwrapped_type<decltype(ctre::match<"abc|def">)>("abc|def");
    print_unwrapped_type<decltype(ctre::match<"(abc|def)">)>("(abc|def)");
    print_unwrapped_type<decltype(ctre::match<"(abc|def)x">)>("(abc|def)x");
    print_unwrapped_type<decltype(ctre::match<"x(abc|def)y">)>("x(abc|def)y");

    return 0;
}
