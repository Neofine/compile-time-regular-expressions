# CTRE Benchmarking

Benchmarks comparing CTRE-SIMD against original CTRE, CTRE-Scalar, RE2, PCRE2, Hyperscan, and std::regex.

## System Requirements

**Tested on:**
- Linux x86_64 with AVX2/SSE4.2 support
- GCC 11+ or Clang 14+
- Python 3.8+ with matplotlib, pandas, seaborn, numpy

**Dependencies:**
- RE2, PCRE2, Hyperscan libraries (pre-built in `lib/`)
- CMake 3.16+

## Quick Start

```bash
# Build benchmarks (from repo root)
cd benchmarking
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
cd ..

# Run all benchmarks
export LD_LIBRARY_PATH="$PWD/lib/lib:$LD_LIBRARY_PATH"

CATEGORIES="simple complex scaling realworld small large fallback adversarial instantiation"
for cat in $CATEGORIES; do
    mkdir -p output/$cat
    ./build/bench_simd $cat > output/$cat/simd.csv
    ./build/bench_original $cat > output/$cat/original.csv
    ./build/bench_scalar $cat > output/$cat/scalar.csv
done

# Generate plots
python3 generate.py
```

## Output

Plots are saved to `output/figures/<category>/`:
- `heatmap.png` — Speedup comparison across all engines
- `bar_comparison.png` — Bar chart at reference input size
- `*_time.png` — Time series per pattern across input sizes

## Benchmark Categories

| Category | Description |
|----------|-------------|
| `simple` | Basic character classes (`[a-z]+`, `[0-9]+`) |
| `complex` | Real patterns (URLs, emails, HTTP headers) |
| `scaling` | Pattern complexity scaling |
| `realworld` | Production-like workloads |
| `small` | Small inputs (1-16 bytes) — SIMD overhead test |
| `large` | Large inputs (1KB-64KB) — throughput test |
| `fallback` | Patterns requiring fallback (backrefs, lookahead) |
| `adversarial` | Patterns unfavorable for SIMD |
| `instantiation` | Regex compilation time |

## Interpreting Results

- **Speedup > 1.0**: SIMD is faster than baseline
- **Speedup < 1.0**: SIMD is slower (overhead)
- Heatmaps use pink (slower) → beige (same) → blue (faster)

## Adding New Patterns

Edit `benchmarks/thesis_benchmark.cpp`:

```cpp
benchmark_pattern<"your_pattern">(
    "Category",           // Category name
    "pattern_name",       // Short identifier
    "description",        // For plots
    gen_input_function,   // Input generator
    SIZES                 // Input sizes to test
);
```

Then rebuild and rerun benchmarks.

## Notes

- All engines use anchored matching (`^pattern$`)
- PCRE2 uses JIT compilation for fair comparison
- std::regex uses `std::regex::optimize`
- Each measurement uses 10,000 iterations after warmup

