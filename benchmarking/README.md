# CTRE Benchmarking

Benchmarks comparing CTRE-SIMD against original CTRE, CTRE-Scalar, RE2, PCRE2, Hyperscan, and std::regex.

## Structure

```
benchmarking/
├── src/              # C++ benchmark source
│   ├── benchmark.cpp
│   └── patterns.hpp
├── scripts/          # Shell scripts
│   ├── build.sh      # Build executables
│   ├── run.sh        # Run benchmarks
│   ├── setup.sh      # System setup (CPU isolation)
│   ├── restore.sh    # Restore system settings
│   ├── codesize.sh   # Measure binary sizes
│   ├── compile_time.sh
│   ├── sysinfo.sh
│   └── common.sh
├── plots/            # Python plotting library
│   ├── style.py
│   ├── loader.py
│   └── figures.py
├── docs/             # LaTeX documentation
├── generate.py       # Main plot generator
├── stats.py          # Statistical aggregation
└── README.md
```

## Requirements

- Linux x86_64 with AVX2/SSE4.2
- GCC 11+ or Clang 14+
- Python 3.8+ with matplotlib, pandas, seaborn, numpy
- RE2, PCRE2, Hyperscan libraries

## Quick Start

```bash
# Build
bash scripts/build.sh

# Run benchmarks
CATEGORIES="simple complex scaling realworld NonMatch"
for cat in $CATEGORIES; do
    mkdir -p output/$cat
    ./build/bench_simd "$cat" > output/$cat/simd.csv
    ./build/bench_scalar "$cat" > output/$cat/scalar.csv  
    ./build/bench_original "$cat" > output/$cat/original.csv
done

# Generate plots
python3 generate.py
```

## Publication-Quality Benchmarks

```bash
# Setup (requires root)
sudo bash scripts/setup.sh

# Run with statistics
BENCHMARK_RUNS=10 bash scripts/run.sh

# Generate plots
python3 generate.py

# Restore system
sudo bash scripts/restore.sh
```

## Categories

| Category | Description |
|----------|-------------|
| `simple` | Basic character classes |
| `complex` | Real patterns (URLs, emails) |
| `scaling` | Pattern complexity scaling |
| `realworld` | Production workloads |
| `small` | Small inputs (1-16 bytes) |
| `large` | Large inputs (1KB-64KB) |
| `fallback` | Patterns requiring fallback |
| `adversarial` | SIMD-unfavorable patterns |
| `instantiation` | Compilation time |
| `NonMatch` | Non-matching inputs |
| `arm` | ARM patterns (ARM only) |
| `arm_nomatch` | ARM non-matching (ARM only) |

## Output

Plots are saved to `output/figures/<category>/`:
- `heatmap.png` — Speedup comparison
- `bar_comparison.png` — Bar chart
- `*_time.png` — Time series

## Adding Patterns

Edit `src/benchmark.cpp`:

```cpp
benchmark_pattern<"[0-9]+">("Category", "name", "[0-9]+", gen_digits, SIZES);
```

Rebuild and rerun.
