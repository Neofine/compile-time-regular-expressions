# CTRE-SIMD

SIMD extensions to [CTRE](https://github.com/hanickadot/compile-time-regular-expressions) (Compile-Time Regular Expressions).

## File Overview

### SIMD Matching (`include/ctre/`)
```
simd_detection.hpp          - CPU feature detection, size thresholds
simd_character_classes.hpp  - [a-z]+, [0-9]+ via range comparison
simd_multirange.hpp         - [a-zA-Z0-9]+ via parallel range checks
simd_shufti.hpp             - Sparse sets [aeiou]+ via shuffle LUT
simd_repetition.hpp         - Single character a+, b*
simd_heuristics.hpp         - Compile-time eligibility checks
simd_literal_search.hpp     - SIMD substring search
simd_string_matching.hpp    - Fixed literal matching
```

### Pattern Analysis (`include/ctre/`)
```
glushkov_nfa.hpp            - AST → position NFA
dominator_analysis.hpp      - Extract required literals (all paths must contain)
region_analysis.hpp         - Extract common suffixes from alternations
decomposition.hpp           - Public API for literal extraction
pattern_traits.hpp          - Type traits for pattern introspection
```

### BitNFA Engine (`include/ctre/bitnfa/`)
```
bitnfa_match.hpp            - Entry points: match(), search()
compiler.hpp                - AST → NFA transitions
state_mask.hpp              - 128-bit state vector operations
simd_acceleration.hpp       - SIMD helpers for state updates
```

### Modified Original Files
```
evaluation.hpp    - SIMD dispatch for repetitions (lines 465-680, 850-930)
wrapper.hpp       - BitNFA dispatch for alternations (lines 87-100, 160-175)
```

## Where to Start Reading

1. **Dispatch logic**: `evaluation.hpp:570` - SIMD eligibility checks and dispatch
2. **Core SIMD**: `simd_character_classes.hpp` - vectorized range matching loop
3. **Pattern analysis**: `glushkov_nfa.hpp` - regex AST to position NFA
4. **Literal extraction**: `dominator_analysis.hpp` - required literal identification

## Running Benchmarks

```bash
# Full suite - builds, runs all benchmarks, generates plots
cd benchmarking && bash scripts/bench_all.sh

# Individual steps:
bash scripts/build.sh           # Build benchmark executables
bash scripts/run.sh             # Run runtime benchmarks → CSV
bash scripts/compile_time.sh    # Measure compile time
bash scripts/codesize.sh        # Measure binary sizes
python3 generate.py             # Generate all plots

# Publication-quality benchmarks (requires root for CPU isolation)
sudo bash scripts/setup.sh      # Configure system (disable turbo, set governor)
BENCHMARK_RUNS=10 bash scripts/run.sh
sudo bash scripts/restore.sh    # Restore system settings
```

## Benchmark Categories

| Category | Description |
|----------|-------------|
| `simple` | Single character classes: `[0-9]+`, `[a-z]+` |
| `complex` | Combined patterns: `[a-z]+[0-9]+`, decimals |
| `realworld` | Validation: IPv4, UUID, email, dates |
| `nomatch` | Non-matching input rejection |
| `fallback` | Patterns requiring scalar fallback |
| `small` | Small inputs (1-16 bytes) |
| `large` | Large inputs (32KB-8MB) |
| `adversarial` | SIMD-unfavorable patterns |

## Output Structure

```
benchmarking/output/
├── simple/simd.csv, scalar.csv, original.csv
├── complex/...
├── realworld/...
├── nomatch/...
├── codesize.csv
├── compile_time.csv
└── figures/
    ├── simple/heatmap.png, *_time.png
    ├── complex/...
    ├── overview/simd_delta_by_size.png
    ├── compile_time/compile_time_by_category.png
    └── statistical/cv_histogram.png
```

## Tests

```bash
g++ -std=c++20 -mavx2 -I include tests/test_glushkov.cpp -o test && ./test
```

Relevant test files:
- `test_glushkov.cpp` - NFA construction
- `test_dominators.cpp` - Literal extraction
- `test_region_analysis.cpp` - Suffix extraction
- `test_char_class_expansion.cpp` - Character class handling
- `integration_test_*.cpp` - End-to-end correctness

## Build Requirements

- GCC 11+ or Clang 14+ with C++20 support
- x86-64 with AVX2 (for SIMD paths; SSE4.2 fallback available)
- RE2, PCRE2, Hyperscan libraries (for comparative benchmarks)
- Python 3.8+, matplotlib, pandas, seaborn, numpy (for plotting)
