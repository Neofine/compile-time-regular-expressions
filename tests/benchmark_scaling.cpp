// Comprehensive scaling benchmark - tests patterns across power-of-2 input sizes
#include <benchmark/benchmark.h>
#include <string>
#include <string_view>

// Include our optimized CTRE
#include "../include/ctre.hpp"

// Test patterns and their minimum input sizes
struct PatternTest {
    const char* name;
    size_t min_size;  // Minimum valid input size (power of 2)
    char fill_char;   // Character to fill the input with
};

const PatternTest patterns[] = {
    // Simple repetitions
    {"a+", 1, 'a'},
    {"a*", 1, 'a'},
    {"b+", 1, 'b'},
    {"b*", 1, 'b'},
    {"z+", 1, 'z'},
    {"z*", 1, 'z'},

    // Character classes
    {"[a-z]+", 1, 'x'},
    {"[a-z]*", 1, 'x'},
    {"[A-Z]+", 1, 'X'},
    {"[A-Z]*", 1, 'X'},
    {"[0-9]+", 1, '5'},
    {"[0-9]*", 1, '5'},

    // Multi-range
    {"[a-zA-Z]+", 1, 'x'},
    {"[a-zA-Z0-9]+", 1, 'x'},
    {"[0-9a-fA-F]+", 1, 'a'},  // Hex digits

    // Sparse sets
    {"[02468]+", 1, '2'},
    {"[13579]+", 1, '3'},
    {"[aeiou]+", 1, 'a'},
    {"[AEIOU]+", 1, 'A'},
};

// Power-of-2 sizes from 1 to 16384 (16KB)
const size_t sizes[] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384};

// Benchmark for a+ pattern
static void BM_aplus(benchmark::State& state) {
    const size_t size = state.range(0);
    std::string input(size, 'a');
    std::string_view sv(input);

    for (auto _ : state) {
        auto result = ctre::match<"a+">(sv);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations() * size);
}

// Benchmark for a* pattern
static void BM_astar(benchmark::State& state) {
    const size_t size = state.range(0);
    std::string input(size, 'a');
    std::string_view sv(input);

    for (auto _ : state) {
        auto result = ctre::match<"a*">(sv);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations() * size);
}

// Benchmark for [a-z]+ pattern
static void BM_az_plus(benchmark::State& state) {
    const size_t size = state.range(0);
    std::string input(size, 'x');
    std::string_view sv(input);

    for (auto _ : state) {
        auto result = ctre::match<"[a-z]+">(sv);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations() * size);
}

// Benchmark for [a-z]* pattern
static void BM_az_star(benchmark::State& state) {
    const size_t size = state.range(0);
    std::string input(size, 'x');
    std::string_view sv(input);

    for (auto _ : state) {
        auto result = ctre::match<"[a-z]*">(sv);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations() * size);
}

// Benchmark for [A-Z]+ pattern
static void BM_AZ_plus(benchmark::State& state) {
    const size_t size = state.range(0);
    std::string input(size, 'X');
    std::string_view sv(input);

    for (auto _ : state) {
        auto result = ctre::match<"[A-Z]+">(sv);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations() * size);
}

// Benchmark for [A-Z]* pattern
static void BM_AZ_star(benchmark::State& state) {
    const size_t size = state.range(0);
    std::string input(size, 'X');
    std::string_view sv(input);

    for (auto _ : state) {
        auto result = ctre::match<"[A-Z]*">(sv);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations() * size);
}

// Benchmark for [0-9]+ pattern
static void BM_09_plus(benchmark::State& state) {
    const size_t size = state.range(0);
    std::string input(size, '5');
    std::string_view sv(input);

    for (auto _ : state) {
        auto result = ctre::match<"[0-9]+">(sv);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations() * size);
}

// Benchmark for [0-9]* pattern
static void BM_09_star(benchmark::State& state) {
    const size_t size = state.range(0);
    std::string input(size, '5');
    std::string_view sv(input);

    for (auto _ : state) {
        auto result = ctre::match<"[0-9]*">(sv);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations() * size);
}

// Benchmark for [a-zA-Z]+ pattern
static void BM_azAZ_plus(benchmark::State& state) {
    const size_t size = state.range(0);
    std::string input(size, 'x');
    std::string_view sv(input);

    for (auto _ : state) {
        auto result = ctre::match<"[a-zA-Z]+">(sv);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations() * size);
}

// Benchmark for [a-zA-Z0-9]+ pattern
static void BM_azAZ09_plus(benchmark::State& state) {
    const size_t size = state.range(0);
    std::string input(size, 'x');
    std::string_view sv(input);

    for (auto _ : state) {
        auto result = ctre::match<"[a-zA-Z0-9]+">(sv);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations() * size);
}

// Benchmark for [02468]+ pattern (sparse)
static void BM_even_plus(benchmark::State& state) {
    const size_t size = state.range(0);
    std::string input(size, '2');
    std::string_view sv(input);

    for (auto _ : state) {
        auto result = ctre::match<"[02468]+">(sv);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations() * size);
}

// Benchmark for [aeiou]+ pattern (sparse)
static void BM_vowel_plus(benchmark::State& state) {
    const size_t size = state.range(0);
    std::string input(size, 'a');
    std::string_view sv(input);

    for (auto _ : state) {
        auto result = ctre::match<"[aeiou]+">(sv);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations() * size);
}

// Register benchmarks for all sizes
BENCHMARK(BM_aplus)->DenseRange(1, 16384, [](size_t& val) { val = (val == 0) ? 1 : val * 2; })->Unit(benchmark::kNanosecond);
BENCHMARK(BM_astar)->DenseRange(1, 16384, [](size_t& val) { val = (val == 0) ? 1 : val * 2; })->Unit(benchmark::kNanosecond);
BENCHMARK(BM_az_plus)->DenseRange(1, 16384, [](size_t& val) { val = (val == 0) ? 1 : val * 2; })->Unit(benchmark::kNanosecond);
BENCHMARK(BM_az_star)->DenseRange(1, 16384, [](size_t& val) { val = (val == 0) ? 1 : val * 2; })->Unit(benchmark::kNanosecond);
BENCHMARK(BM_AZ_plus)->DenseRange(1, 16384, [](size_t& val) { val = (val == 0) ? 1 : val * 2; })->Unit(benchmark::kNanosecond);
BENCHMARK(BM_AZ_star)->DenseRange(1, 16384, [](size_t& val) { val = (val == 0) ? 1 : val * 2; })->Unit(benchmark::kNanosecond);
BENCHMARK(BM_09_plus)->DenseRange(1, 16384, [](size_t& val) { val = (val == 0) ? 1 : val * 2; })->Unit(benchmark::kNanosecond);
BENCHMARK(BM_09_star)->DenseRange(1, 16384, [](size_t& val) { val = (val == 0) ? 1 : val * 2; })->Unit(benchmark::kNanosecond);
BENCHMARK(BM_azAZ_plus)->DenseRange(1, 16384, [](size_t& val) { val = (val == 0) ? 1 : val * 2; })->Unit(benchmark::kNanosecond);
BENCHMARK(BM_azAZ09_plus)->DenseRange(1, 16384, [](size_t& val) { val = (val == 0) ? 1 : val * 2; })->Unit(benchmark::kNanosecond);
BENCHMARK(BM_even_plus)->DenseRange(1, 16384, [](size_t& val) { val = (val == 0) ? 1 : val * 2; })->Unit(benchmark::kNanosecond);
BENCHMARK(BM_vowel_plus)->DenseRange(1, 16384, [](size_t& val) { val = (val == 0) ? 1 : val * 2; })->Unit(benchmark::kNanosecond);

BENCHMARK_MAIN();
