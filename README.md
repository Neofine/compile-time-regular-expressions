# CTRE-SIMD

SIMD extensions to [CTRE](https://github.com/hanickadot/compile-time-regular-expressions) (Compile-Time Regular Expressions).

## What Was Modified

This implementation adds ~6,500 lines to the original CTRE library:

| Component | Files | Lines | Purpose |
|-----------|-------|-------|---------|
| SIMD matching | `simd_*.hpp` (8 files) | ~3,300 | Vectorized character class matching |
| Pattern analysis | 5 files | ~1,700 | NFA construction, literal extraction |
| BitNFA engine | `bitnfa/` (12 files) | ~1,500 | Parallel state simulation |

**Original CTRE files are unmodified** except for dispatch logic in `evaluation.hpp` (~20 lines) and `wrapper.hpp` (~15 lines).

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
# Quick test (~2 min) - compares SIMD vs scalar for 37 patterns
./run_individual_benchmarks.sh

# Full suite (~5 min) - generates CSV data and PNG plots
./benchmarking/scripts/generate_all.sh

# Individual steps:
./benchmarking/scripts/build.sh              # Compile binaries
./benchmarking/scripts/run_benchmarks.sh     # Run benchmarks → CSV
./benchmarking/scripts/measure_codesize.sh   # Measure binary size
./benchmarking/scripts/measure_compile_time.sh  # Measure compile time
python3 benchmarking/generate.py             # Generate plots

# Cleanup all generated files
./cleanup.sh
```

## Output Structure

```
benchmarking/output/
├── simple/simd.csv, baseline.csv
├── complex/simd.csv, baseline.csv
├── scaling/simd.csv, baseline.csv
├── realworld/simd.csv, baseline.csv
├── codesize.csv
├── compile_time.csv
└── figures/*.png
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

- GCC 10+ or Clang 12+ with C++20 support
- x86-64 with AVX2 (for SIMD paths)
- Python 3.8+, matplotlib, pandas, seaborn (for plotting only)
