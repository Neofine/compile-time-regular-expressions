# CTRE-SIMD Implementation

This repository contains the implementation described in the thesis. The code extends [CTRE](https://github.com/hanickadot/compile-time-regular-expressions) with SIMD acceleration.

## Code Location

### SIMD Character Class Matching (Section 2.3)

```
include/ctre/simd_character_classes.hpp  - Core range checking [a-z]+, [0-9]+
include/ctre/simd_multirange.hpp         - Multi-range [a-zA-Z0-9]+
include/ctre/simd_shufti.hpp             - Shufti algorithm for sparse sets
include/ctre/simd_repetition.hpp         - Single char repetition a+, b*
include/ctre/simd_detection.hpp          - Runtime SIMD detection
```

### Pattern Analysis & Prefiltering (Section 2.4)

```
include/ctre/glushkov_nfa.hpp            - Glushkov NFA construction
include/ctre/dominator_analysis.hpp      - Dominator-based literal extraction
include/ctre/region_analysis.hpp         - Common suffix extraction
include/ctre/decomposition.hpp           - Prefiltering API
```

### BitNFA Engine (Section 2.5)

```
include/ctre/bitnfa/                     - Parallel bit-state NFA (12 files)
include/ctre/bitnfa/bitnfa_match.hpp     - Main matching logic
include/ctre/bitnfa/state_mask.hpp       - SIMD state representation
```

### Integration Points

```
include/ctre/evaluation.hpp              - SIMD dispatch (line ~180)
include/ctre/wrapper.hpp                 - BitNFA integration (line ~80)
```

## Running Benchmarks

```bash
# Quick benchmark (2 min, 31 patterns)
./run_individual_benchmarks.sh

# Full benchmark suite
cd plots
./scripts/build.sh
./scripts/run_benchmarks.sh
python3 generate.py   # generates plots/output/
```

## Tests

```bash
# Compile a test
g++ -std=c++20 -I include tests/test_glushkov.cpp -o test && ./test
```

Key test files:
- `tests/test_glushkov.cpp` - NFA construction
- `tests/test_dominators.cpp` - Literal extraction
- `tests/test_region_analysis.cpp` - Suffix extraction
