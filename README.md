# CTRE-SIMD

SIMD-accelerated extensions to [CTRE](https://github.com/hanickadot/compile-time-regular-expressions) (Compile-Time Regular Expressions).

## Features

- **Drop-in replacement**: Works with existing CTRE patterns
- **Automatic SIMD dispatch**: Vectorized matching for eligible patterns
- **Zero runtime overhead for ineligible patterns**: Falls back to scalar CTRE
- **AVX2/SSE4.2 support** with scalar fallback for other architectures

## Usage

```cpp
#include <ctre.hpp>

// Works exactly like CTRE - SIMD acceleration is automatic
auto result = ctre::match<"[a-z]+">(input);
auto found = ctre::search<"[0-9]+@[a-z]+\\.com">(input);

// Patterns that benefit from SIMD:
// - Character class repetitions: [a-z]+, [0-9]+, [a-zA-Z0-9]+
// - Single character repetitions: a+, x*
// - Sparse character sets: [aeiou]+
// - Patterns with required literals for prefiltering
```

## Installation

Header-only. Copy `include/` to your project or add to include path:

```bash
g++ -std=c++20 -O3 -march=native -I/path/to/ctre-simd/include your_code.cpp
```

## Requirements

- C++20 compiler (GCC 11+, Clang 14+)
- x86-64 with AVX2 for best performance (SSE4.2 and scalar fallbacks available)

## Optimized Patterns

| Pattern Type | Example | Optimization |
|--------------|---------|--------------|
| Character ranges | `[a-z]+`, `[0-9]+` | AVX2 range comparison |
| Multi-range | `[a-zA-Z0-9]+` | Parallel range checks |
| Sparse sets | `[aeiou]+` | Shufti algorithm |
| Single char | `a+`, `x*` | SIMD broadcast + compare |
| Alternations | `(foo\|bar)` | BitNFA engine |
| Literal prefilter | `.*test.*` | SIMD literal search |

## Benchmarks

```bash
cd benchmarking && ./build.sh && ./build/bench_simd
```

Requires RE2, PCRE2, Hyperscan for comparisons.

## Tests

```bash
cd tests && bash run_tests.sh
```

## Project Structure

```
include/
├── ctre.hpp                    # Main include (use this)
├── ctre/
│   ├── simd_*.hpp              # SIMD matching implementations
│   ├── glushkov_nfa.hpp        # Pattern analysis
│   ├── dominator_analysis.hpp  # Literal extraction
│   └── bitnfa/                 # BitNFA engine for alternations
└── ctll/                       # Compile-time library (from CTRE)
```

## License

Apache 2.0 (same as CTRE)
