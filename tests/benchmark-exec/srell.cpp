#include "common.hpp"

int main (int argc, char ** argv)
{
	srell::regex r(PATTERN);
	
	return benchmark(argc, argv, [&r](std::string_view line) -> bool {
		const std::string str(line);
		return srell::regex_search(str, r);
	});
}