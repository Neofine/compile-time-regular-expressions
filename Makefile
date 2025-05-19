.PHONY: default all clean grammar compare single-header single-header/ctre.hpp single-header/ctre-unicode.hpp single-header/unicode-db.hpp benchmark-exec benchmark-compare

default: all
	
TARGETS := $(wildcard tests/benchmark-exec/*.cpp)
IGNORE := $(wildcard tests/benchmark/*.cpp) $(wildcard tests/benchmark-exec/*.cpp)

DESATOMAT := /bin/false

CXX_STANDARD := 20

PYTHON := python3.9

PEDANTIC:=-pedantic

override CXXFLAGS := $(CXXFLAGS) -std=c++$(CXX_STANDARD) -Iinclude -O3 $(PEDANTIC) -Wall -Wextra -Werror -Wconversion
LDFLAGS := 

TESTS := $(wildcard tests/*.cpp) $(wildcard tests/benchmark/*.cpp)
TRUE_TARGETS := $(TARGETS:%.cpp=%)
override TRUE_TARGETS := $(filter-out $(IGNORE:%.cpp=%), $(TRUE_TARGETS))
OBJECTS := $(TARGETS:%.cpp=%.o) $(TESTS:%.cpp=%.o)
override OBJECTS := $(filter-out $(IGNORE:%.cpp=%.o),$(OBJECTS))
DEPEDENCY_FILES := $(OBJECTS:%.o=%.d)

all: $(TRUE_TARGETS) $(OBJECTS)
	
list:
	echo $(SUPPORTED_CPP20)
	
$(TRUE_TARGETS): %: %.o
	$(CXX)  $< $(LDFLAGS) -o $@ 
	
$(OBJECTS): %.o: %.cpp
	$(CXX) $(CXXFLAGS) -MMD -c $< -o $@

-include $(DEPEDENCY_FILES)

benchmark:
	@echo ""
	@echo "Available benchmark targets:"
	@echo "  make benchmark-simple    - Run simplified benchmark without external dependencies"
	@echo "  make benchmark-external  - Run full benchmarks with external libraries (RE2, Boost, PCRE2, etc.)"
	@echo "  make benchmark-compare   - Run comparative benchmarks and show results in CSV format"
	@echo ""
	@echo "Running simplified benchmark by default..."
	@echo ""
	@$(CXX) $(CXXFLAGS) -o simple_benchmark simple_benchmark.cpp
	@./simple_benchmark && rm simple_benchmark

benchmark-simple:
	@$(MAKE) clean
	@echo ""
	@echo "Running simplified benchmark without external dependencies..."
	@echo ""
	@$(CXX) $(CXXFLAGS) -o simple_benchmark simple_benchmark.cpp
	@./simple_benchmark && rm simple_benchmark

benchmark-external:
	@$(MAKE) clean
	@echo ""
	@echo "Running full benchmarks with external dependencies..."
	@echo ""
	@cd tests/benchmark-exec && CXXFLAGS="-std=c++$(CXX_STANDARD) -O3 -I../../include -I/opt/homebrew/opt/re2/include -I/opt/homebrew/Cellar/abseil/20240722.1/include -I/opt/homebrew/opt/boost/include -I/opt/homebrew/opt/pcre2/include -Wall -Wextra -Wno-error -Wno-sign-conversion -Wno-conversion" $(MAKE)
	@echo ""
	@echo "Running baseline benchmark..."
	@cd tests/benchmark-exec && ./baseline input.txt
	@echo ""
	@echo "Running CTRE benchmark..."
	@cd tests/benchmark-exec && ./ctre input.txt
	@echo ""
	@echo "Running RE2 benchmark..."
	@cd tests/benchmark-exec && ./re2 input.txt
	@echo ""
	@echo "Running Boost benchmark..."
	@cd tests/benchmark-exec && ./boost input.txt
	@echo ""
	@echo "Running PCRE2 benchmark..."
	@cd tests/benchmark-exec && ./pcre input.txt
	@echo ""
	@echo "Running std::regex benchmark..."
	@cd tests/benchmark-exec && ./std input.txt
	@echo ""
	@echo "Running Boost Xpressive benchmark..."
	@cd tests/benchmark-exec && ./xpressive input.txt
	@echo ""
	@echo "Running SRELL benchmark..."
	@cd tests/benchmark-exec && ./srell input.txt

benchmark-compare:
	@$(MAKE) clean
	@echo ""
	@echo "Running comparative benchmarks in benchmark mode..."
	@echo ""
	@cd tests/benchmark-exec && CXXFLAGS="-std=c++$(CXX_STANDARD) -O3 -I../../include -I/opt/homebrew/opt/re2/include -I/opt/homebrew/Cellar/abseil/20240722.1/include -I/opt/homebrew/opt/boost/include -I/opt/homebrew/opt/pcre2/include -Wall -Wextra -Wno-error -Wno-sign-conversion -Wno-conversion" $(MAKE)
	@echo "library;pattern;duration" > tests/benchmark-exec/result.csv
	@cd tests/benchmark-exec && ./baseline input.txt benchmark "Baseline" 2>/dev/null >>../benchmark-exec/result.csv
	@cd tests/benchmark-exec && ./ctre input.txt benchmark "CTRE" 2>/dev/null >>../benchmark-exec/result.csv
	@cd tests/benchmark-exec && ./re2 input.txt benchmark "RE2" 2>/dev/null >>../benchmark-exec/result.csv
	@cd tests/benchmark-exec && ./boost input.txt benchmark "Boost::Regex" 2>/dev/null >>../benchmark-exec/result.csv
	@cd tests/benchmark-exec && ./pcre input.txt benchmark "PCRE2" 2>/dev/null >>../benchmark-exec/result.csv
	@cd tests/benchmark-exec && ./std input.txt benchmark "std::regex" 2>/dev/null >>../benchmark-exec/result.csv
	@cd tests/benchmark-exec && ./xpressive input.txt benchmark "Boost::Xpressive" 2>/dev/null >>../benchmark-exec/result.csv
	@cd tests/benchmark-exec && ./srell input.txt benchmark "SRELL" 2>/dev/null >>../benchmark-exec/result.csv
	@echo ""
	@echo "Benchmark Results (Library;Pattern;Duration in ms):"
	@echo "------------------------------------------------"
	@cat tests/benchmark-exec/result.csv
	
benchmark-clean:
	@$(MAKE) IGNORE="" clean
	@cd tests/benchmark-exec && $(MAKE) clean
	@rm -f tests/benchmark-exec/result.csv tests/benchmark-exec/header.csv

benchmark-exec:
	@cd tests/benchmark-exec && CXXFLAGS="-std=c++$(CXX_STANDARD) -O3 -I../../include -I/opt/homebrew/opt/re2/include -I/opt/homebrew/Cellar/abseil/20240722.1/include -I/opt/homebrew/opt/boost/include -Wall -Wextra -Wno-error -Wno-sign-conversion -Wno-conversion" $(MAKE)

clean:
	rm -f $(TRUE_TARGETS) $(OBJECTS) $(DEPEDENCY_FILES) mtent12.txt mtent12.zip
	
grammar: include/ctre/pcre.hpp
	
regrammar: 
	@rm -f include/ctre/pcre.hpp
	@$(MAKE) grammar
	
include/ctre/pcre.hpp: include/ctre/pcre.gram
	@echo "LL1q $<"
	@$(DESATOMAT) --ll --q --input=include/ctre/pcre.gram --output=include/ctre/ --generator=cpp_ctll_v2  --cfg:fname=pcre.hpp --cfg:namespace=ctre --cfg:guard=CTRE__PCRE__HPP --cfg:grammar_name=pcre
	
mtent12.zip:
	curl -s http://www.gutenberg.org/files/3200/old/mtent12.zip -o mtent12.zip
	
mtent12.txt: mtent12.zip
	unzip -o mtent12.zip
	touch mtent12.txt

single-header: single-header/ctre.hpp single-header/ctre-unicode.hpp single-header/unicode-db.hpp

single-header/unicode-db.hpp: include/unicode-db/unicode-db.hpp
	cp $+ $@

single-header/ctre.hpp:
	${PYTHON} -m quom include/ctre.hpp ctre.hpp.tmp
	echo "/*" > single-header/ctre.hpp
	cat LICENSE >> single-header/ctre.hpp
	echo "*/" >> single-header/ctre.hpp
	cat ctre.hpp.tmp >> single-header/ctre.hpp
	rm ctre.hpp.tmp

single-header/ctre-unicode.hpp:
	${PYTHON} -m quom include/ctre-unicode.hpp ctre-unicode.hpp.tmp
	echo "/*" > single-header/ctre-unicode.hpp
	cat LICENSE >> single-header/ctre-unicode.hpp
	echo "*/" >> single-header/ctre-unicode.hpp
	cat ctre-unicode.hpp.tmp >> single-header/ctre-unicode.hpp
	rm ctre-unicode.hpp.tmp
	
REPEAT:=10

compare: mtent12.txt
	$(CXX) $(CXXFLAGS) -MMD -march=native -DPATTERN="\"(${PATTERN})\"" -c tests/benchmark-range/measurement.cpp -o tests/benchmark-range/measurement.o
	$(CXX) tests/benchmark-range/measurement.o -lboost_regex -lpcre2-8 -lre2 -o tests/benchmark-range/measurement
	tests/benchmark-range/measurement all mtent12.txt ${REPEAT}
