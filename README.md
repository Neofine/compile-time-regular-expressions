# CTRE-SIMD: SIMD-Accelerated Compile-Time Regular Expressions

This is a fork of [CTRE](https://github.com/hanickadot/compile-time-regular-expressions) with SIMD acceleration for character class repetitions and pattern-specific optimizations.

## Key Modifications

### SIMD Character Class Matching (`include/ctre/simd_*.hpp`)
- `simd_character_classes.hpp` - AVX2/SSE4.2 range checking for `[a-z]+`, `[0-9]+`
- `simd_multirange.hpp` - Multi-range support for `[a-zA-Z0-9]+`
- `simd_shufti.hpp` - Shufti algorithm for sparse sets like `[aeiou]+`
- `simd_repetition.hpp` - Single character repetition (`a+`, `b*`)
- `simd_detection.hpp` - Runtime SIMD capability detection

### Pattern Analysis (`include/ctre/`)
- `glushkov_nfa.hpp` - Glushkov NFA construction for pattern analysis
- `dominator_analysis.hpp` - Extract required literals for prefiltering
- `region_analysis.hpp` - Common suffix extraction from alternations
- `decomposition.hpp` - Public API for literal extraction
- `pattern_traits.hpp` - Consolidated type traits for patterns

### BitNFA Engine (`include/ctre/bitnfa/`)
- Parallel bit-state NFA for alternation patterns (`a|b|c`)
- 15-39% faster than standard evaluation for alternations

### Modified Core Files
- `include/ctre/evaluation.hpp` - SIMD dispatch and early rejection
- `include/ctre/wrapper.hpp` - BitNFA integration for alternations

## Quick Benchmark

```bash
./run_individual_benchmarks.sh
```

This compiles 81 patterns with and without SIMD (`-DCTRE_DISABLE_SIMD`) and reports speedups.

**Typical results:**
- Character class repetitions: **4-24x speedup**
- Single char repetitions: **5-16x speedup**
- Alternations (BitNFA): **1.1-1.3x speedup**

## Full Benchmarks

```bash
cd plots

# Build benchmark binary
./scripts/build.sh

# Run all benchmarks (generates CSV files)
./scripts/run_benchmarks.sh

# Generate plots
python3 generate.py
```

**Output:**
- `plots/output/` - CSV benchmark data
- `plots/output/figures/` - PNG visualizations

## Project Structure

```
include/ctre/
├── simd_*.hpp          # SIMD implementations
├── bitnfa/             # BitNFA engine
├── glushkov_nfa.hpp    # NFA construction
├── decomposition.hpp   # Literal extraction API
└── pattern_traits.hpp  # Type traits

plots/
├── benchmarks/         # Benchmark source (thesis_benchmark.cpp)
├── plotting/           # Python plotting code
├── scripts/            # Build/run scripts
└── output/             # Generated results

thesis/                 # LaTeX documentation
results/individual/     # Quick benchmark results
```

## Requirements

- C++20 compiler (GCC 10+, Clang 12+)
- x86-64 with AVX2 (falls back to SSE4.2 or scalar)
- Python 3.8+ with matplotlib, pandas, seaborn (for plots)

## License

Same as original CTRE - see LICENSE file.

