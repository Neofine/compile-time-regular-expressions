.PHONY: default all clean grammar compare single-header single-header/ctre.hpp single-header/ctre-unicode.hpp single-header/unicode-db.hpp

default: all

TARGETS := $(wildcard tests/benchmark-exec/*.cpp)
IGNORE := $(wildcard tests/benchmark/*.cpp) $(wildcard tests/benchmark-exec/*.cpp)

DESATOMAT := /bin/false

CXX_STANDARD := 20
CXX := g++

PYTHON := python3.9

PEDANTIC:=-pedantic

override CXXFLAGS := $(CXXFLAGS) -std=c++$(CXX_STANDARD) -Iinclude -Isrell_include -O3 $(PEDANTIC) -Wall -Wextra -Werror -Wconversion -march=native -mtune=native -mavx2 -msse4.2 -mfma -mbmi2 -mlzcnt -mpopcnt -funroll-loops -ffast-math -flto
LDFLAGS := -lstdc++ -flto

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
	@$(MAKE) clean
	@$(MAKE) IGNORE=""

benchmark-clean:
	@$(MAKE) IGNORE="" clean

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

# Phase 1: Glushkov NFA Construction (isolated development - safe!)
test_glushkov: tests/test_glushkov
	./tests/test_glushkov

tests/test_glushkov: tests/test_glushkov.cpp
	$(CXX) $(CXXFLAGS) -MMD -c tests/test_glushkov.cpp -o tests/test_glushkov.o
	$(CXX) tests/test_glushkov.o -o tests/test_glushkov

test_dominators: tests/test_dominators
	./tests/test_dominators

tests/test_dominators: tests/test_dominators.cpp tests/test_glushkov.cpp
	$(CXX) $(CXXFLAGS) -MMD -c tests/test_dominators.cpp -o tests/test_dominators.o
	$(CXX) tests/test_dominators.o -o tests/test_dominators

test_fast_search: tests/test_fast_search
	./tests/test_fast_search

tests/test_fast_search: tests/test_fast_search.cpp
	$(CXX) $(CXXFLAGS) -MMD tests/test_fast_search.cpp -o tests/test_fast_search

benchmark_fast_search: tests/benchmark_fast_search
	./tests/benchmark_fast_search

tests/benchmark_fast_search: tests/benchmark_fast_search.cpp
	$(CXX) $(CXXFLAGS) -MMD tests/benchmark_fast_search.cpp -o tests/benchmark_fast_search

# Essential SIMD Tests - Clean and Focused
simd_comprehensive_test: tests/comprehensive_simd_test
	./tests/comprehensive_simd_test

tests/comprehensive_simd_test: tests/comprehensive_simd_test.cpp
	$(CXX) $(CXXFLAGS) -MMD -march=native -c tests/comprehensive_simd_test.cpp -o tests/comprehensive_simd_test.o
	$(CXX) tests/comprehensive_simd_test.o -o tests/comprehensive_simd_test

simd_comprehensive_test_disabled: tests/comprehensive_simd_test_disabled
	./tests/comprehensive_simd_test_disabled

tests/comprehensive_simd_test_disabled: tests/comprehensive_simd_test.cpp
	$(CXX) $(CXXFLAGS) -MMD -march=native -DCTRE_DISABLE_SIMD -c tests/comprehensive_simd_test.cpp -o tests/comprehensive_simd_test_disabled.o
	$(CXX) tests/comprehensive_simd_test_disabled.o -o tests/comprehensive_simd_test_disabled

simd_ab_test: tests/simd_ab_test_final
	./tests/simd_ab_test_final

tests/simd_ab_test_final: tests/simd_ab_test_final.cpp
	$(CXX) $(CXXFLAGS) -MMD -march=native -c tests/simd_ab_test_final.cpp -o tests/simd_ab_test_final.o
	$(CXX) tests/simd_ab_test_final.o -o tests/simd_ab_test_final

simd_ab_test_disabled: tests/simd_ab_test_final_disabled
	./tests/simd_ab_test_final_disabled

tests/simd_ab_test_final_disabled: tests/simd_ab_test_final.cpp
	$(CXX) $(CXXFLAGS) -MMD -march=native -DCTRE_DISABLE_SIMD -c tests/simd_ab_test_final.cpp -o tests/simd_ab_test_final_disabled.o
	$(CXX) tests/simd_ab_test_final_disabled.o -o tests/simd_ab_test_final_disabled

simd_constexpr_test: tests/constexpr_simd_test
	./tests/constexpr_simd_test

tests/constexpr_simd_test: tests/constexpr_simd_test.cpp
	$(CXX) $(CXXFLAGS) -MMD -march=native -c tests/constexpr_simd_test.cpp -o tests/constexpr_simd_test.o
	$(CXX) tests/constexpr_simd_test.o -o tests/constexpr_simd_test

simd_flag_test: tests/simd_flag_test
	./tests/simd_flag_test

tests/simd_flag_test: tests/simd_flag_test.cpp
	$(CXX) $(CXXFLAGS) -MMD -march=native -c tests/simd_flag_test.cpp -o tests/simd_flag_test.o
	$(CXX) tests/simd_flag_test.o -o tests/simd_flag_test

simd_static_test: tests/simd_static_assertions
	./tests/simd_static_assertions

tests/simd_static_assertions: tests/simd_static_assertions.cpp
	$(CXX) $(CXXFLAGS) -MMD -march=native -c tests/simd_static_assertions.cpp -o tests/simd_static_assertions.o
	$(CXX) tests/simd_static_assertions.o -o tests/simd_static_assertions

simd_repetition_test: tests/simd_repetition_test
	./tests/simd_repetition_test

tests/simd_repetition_test: tests/simd_repetition_test.cpp
	$(CXX) $(CXXFLAGS) -MMD -march=native -c tests/simd_repetition_test.cpp -o tests/simd_repetition_test.o
	$(CXX) tests/simd_repetition_test.o -o tests/simd_repetition_test

simd_repetition_benchmark: tests/simd_repetition_benchmark
	./tests/simd_repetition_benchmark

tests/simd_repetition_benchmark: tests/simd_repetition_benchmark.cpp
	$(CXX) $(CXXFLAGS) -MMD -march=native -c tests/simd_repetition_benchmark.cpp -o tests/simd_repetition_benchmark.o
	$(CXX) tests/simd_repetition_benchmark.o -o tests/simd_repetition_benchmark

simd_repetition_benchmark_disabled: tests/simd_repetition_benchmark_disabled
	./tests/simd_repetition_benchmark_disabled

tests/simd_repetition_benchmark_disabled: tests/simd_repetition_benchmark.cpp
	$(CXX) $(CXXFLAGS) -MMD -march=native -DCTRE_DISABLE_SIMD -c tests/simd_repetition_benchmark.cpp -o tests/simd_repetition_benchmark_disabled.o
	$(CXX) tests/simd_repetition_benchmark_disabled.o -o tests/simd_repetition_benchmark_disabled



simd_character_class_benchmark: tests/simd_character_class_benchmark
	./tests/simd_character_class_benchmark

tests/simd_character_class_benchmark: tests/simd_character_class_benchmark.cpp
	$(CXX) $(CXXFLAGS) -MMD -march=native -c tests/simd_character_class_benchmark.cpp -o tests/simd_character_class_benchmark.o
	$(CXX) tests/simd_character_class_benchmark.o -o tests/simd_character_class_benchmark

simd_character_class_benchmark_disabled: tests/simd_character_class_benchmark_disabled
	./tests/simd_character_class_benchmark_disabled

tests/simd_character_class_benchmark_disabled: tests/simd_character_class_benchmark.cpp
	$(CXX) -std=c++20 -Iinclude -Isrell_include -O1 -pedantic -Wall -Wextra -Werror -Wconversion -MMD -march=native -DCTRE_DISABLE_SIMD -c tests/simd_character_class_benchmark.cpp -o tests/simd_character_class_benchmark_disabled.o
	$(CXX) tests/simd_character_class_benchmark_disabled.o -o tests/simd_character_class_benchmark_disabled

simd_vs_nosimd_benchmark: tests/simd_vs_nosimd_benchmark
	./tests/simd_vs_nosimd_benchmark

tests/simd_vs_nosimd_benchmark: tests/simd_vs_nosimd_benchmark.cpp
	$(CXX) $(CXXFLAGS) -MMD -march=native -c tests/simd_vs_nosimd_benchmark.cpp -o tests/simd_vs_nosimd_benchmark.o
	$(CXX) tests/simd_vs_nosimd_benchmark.o -o tests/simd_vs_nosimd_benchmark -lstdc++

simd_vs_nosimd_benchmark_disabled: tests/simd_vs_nosimd_benchmark_disabled
	./tests/simd_vs_nosimd_benchmark_disabled

tests/simd_vs_nosimd_benchmark_disabled: tests/simd_vs_nosimd_benchmark.cpp
	$(CXX) $(CXXFLAGS) -MMD -march=native -DCTRE_DISABLE_SIMD -c tests/simd_vs_nosimd_benchmark.cpp -o tests/simd_vs_nosimd_benchmark_disabled.o
	$(CXX) tests/simd_vs_nosimd_benchmark_disabled.o -o tests/simd_vs_nosimd_benchmark_disabled -lstdc++

single_char_simd_test: tests/single_char_simd_test
	./tests/single_char_simd_test

tests/single_char_simd_test: tests/single_char_simd_test.cpp
	$(CXX) $(CXXFLAGS) -MMD -march=native -c tests/single_char_simd_test.cpp -o tests/single_char_simd_test.o
	$(CXX) tests/single_char_simd_test.o -o tests/single_char_simd_test -lstdc++

minimal_simd_test: tests/minimal_simd_test
	./tests/minimal_simd_test

tests/minimal_simd_test: tests/minimal_simd_test.cpp
	$(CXX) -std=c++20 -O3 -march=native -c tests/minimal_simd_test.cpp -o tests/minimal_simd_test.o
	$(CXX) tests/minimal_simd_test.o -o tests/minimal_simd_test -lstdc++

trait_test: tests/trait_test
	./tests/trait_test

tests/trait_test: tests/trait_test.cpp
	$(CXX) -std=c++20 -O3 -march=native -Iinclude -c tests/trait_test.cpp -o tests/trait_test.o
	$(CXX) tests/trait_test.o -o tests/trait_test -lstdc++

simple_trait_test: tests/simple_trait_test
	./tests/simple_trait_test

tests/simple_trait_test: tests/simple_trait_test.cpp
	$(CXX) -std=c++20 -O3 -march=native -c tests/simple_trait_test.cpp -o tests/simple_trait_test.o
	$(CXX) tests/simple_trait_test.o -o tests/simple_trait_test -lstdc++

debug_ast_structure: tests/debug_ast_structure
	./tests/debug_ast_structure

tests/debug_ast_structure: tests/debug_ast_structure.cpp
	$(CXX) -std=c++20 -O3 -march=native -Iinclude -c tests/debug_ast_structure.cpp -o tests/debug_ast_structure.o
	$(CXX) tests/debug_ast_structure.o -o tests/debug_ast_structure -lstdc++

debug_a_star: tests/debug_a_star
	./tests/debug_a_star

tests/debug_a_star: tests/debug_a_star.cpp
	$(CXX) -std=c++20 -O3 -march=native -Iinclude -c tests/debug_a_star.cpp -o tests/debug_a_star.o
	$(CXX) tests/debug_a_star.o -o tests/debug_a_star -lstdc++

debug_simple_test: tests/debug_simple_test
	./tests/debug_simple_test

tests/debug_simple_test: tests/debug_simple_test.cpp
	$(CXX) -std=c++20 -O3 -march=native -Iinclude -c tests/debug_simple_test.cpp -o tests/debug_simple_test.o
	$(CXX) tests/debug_simple_test.o -o tests/debug_simple_test -lstdc++
