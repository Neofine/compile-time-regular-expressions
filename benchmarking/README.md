# CTRE Benchmarking

Benchmarks comparing CTRE-SIMD against original CTRE, CTRE-Scalar, RE2, PCRE2, Hyperscan, and std::regex.

## System Requirements

**Tested on:**
- Linux x86_64 with AVX2/SSE4.2 support
- GCC 11+ or Clang 14+
- Python 3.8+ with matplotlib, pandas, seaborn, numpy

**Dependencies:**
- RE2, PCRE2, Hyperscan libraries (system packages or pre-built in `lib/`)
- CMake 3.16+ (optional, if not using build.sh)

## Installation

### Install System Libraries (Recommended)

```bash
# Ubuntu/Debian
sudo apt-get install libre2-dev libpcre2-dev libhyperscan-dev

# The build script will automatically detect and use system libraries via pkg-config
```

Alternatively, pre-built libraries can be placed in `lib/lib/` and `lib/include/`.

## Quick Start

### 1. Build Benchmark Executables

```bash
cd benchmarking
bash scripts/build.sh
```

This builds three executables:
- `build/bench_simd` - CTRE-SIMD (full SIMD)
- `build/bench_scalar` - CTRE-Scalar (SIMD disabled, scalar code paths)
- `build/bench_original` - CTRE-Original (upstream CTRE)

### 2. Run All Benchmarks

```bash
# All non-ARM categories (for x86_64 systems)
CATEGORIES="simple complex scaling realworld small large fallback adversarial instantiation NonMatch"

for cat in $CATEGORIES; do
    echo "Running $cat..."
    mkdir -p output/$cat
    ./build/bench_simd "$cat" > output/$cat/simd.csv 2>&1
    ./build/bench_scalar "$cat" > output/$cat/scalar.csv 2>&1
    ./build/bench_original "$cat" > output/$cat/original.csv 2>&1
done
```

**Note:** Category names are case-sensitive. Use `NonMatch` (not `nomatch`) for non-matching patterns.

### 3. Run Additional Benchmarks

#### Code Size Benchmarks

```bash
bash scripts/measure_codesize.sh
```

Generates `output/codesize.csv` comparing binary sizes between baseline and SIMD variants.

#### Compile Time Benchmarks

```bash
bash scripts/measure_compile_time.sh
```

Generates `output/compile_time.csv` comparing compilation times between baseline and SIMD variants.

### 4. Generate All Plots

```bash
python3 generate.py
```

This generates plots for all categories, including:
- Category-specific plots in `output/figures/<category>/`
- Overview plots in `output/figures/overview/`
- Code size plots in `output/figures/codesize/`
- Compile time plots in `output/figures/compile_time/`

**Note:** The overview plot requires merged data files. The script automatically creates `output/simd.csv` and `output/baseline.csv` by merging data from `simple/` and `complex/` categories if they don't exist.

## Output

### Category Plots

Plots are saved to `output/figures/<category>/`:
- `heatmap.png` — Speedup comparison across all engines
- `bar_comparison.png` — Bar chart at reference input size
- `*_time.png` — Time series per pattern across input sizes
- `speedup_bars.png` — Speedup bars (for adversarial category)

### Overview Plots

- `output/figures/overview/simd_delta_by_size.png` — SIMD impact (best vs worst case) across input sizes

### Code Size Plots

- `output/figures/codesize/codesize_comparison.png` — Binary size comparison
- `output/figures/codesize/simd_overhead.png` — SIMD code size overhead percentage

### Compile Time Plots

- `output/figures/compile_time/compile_time_by_category.png` — Average compile time by category
- `output/figures/compile_time/compile_time_per_pattern.png` — Compile time per pattern
- `output/figures/compile_time/compile_overhead_pct.png` — Compile overhead percentage

### Instantiation Time Plot

- `output/figures/instantiation/instantiation_time.png` — Regex compilation/instantiation time comparison

## Benchmark Categories

| Category | Description | Note |
|----------|-------------|------|
| `simple` | Basic character classes (`[a-z]+`, `[0-9]+`) | |
| `complex` | Real patterns (URLs, emails, HTTP headers) | |
| `scaling` | Pattern complexity scaling | May segfault on some systems |
| `realworld` | Production-like workloads | |
| `small` | Small inputs (1-16 bytes) — SIMD overhead test | |
| `large` | Large inputs (1KB-64KB) — throughput test | |
| `fallback` | Patterns requiring fallback (backrefs, lookahead) | |
| `adversarial` | Patterns unfavorable for SIMD | |
| `instantiation` | Regex compilation/instantiation time | |
| `NonMatch` | Non-matching inputs (rejection speed) | **Case-sensitive: use `NonMatch`** |
| `arm` | ARM matching patterns | ARM-specific, skip on x86_64 |
| `arm_nomatch` | ARM non-matching patterns | ARM-specific, skip on x86_64 |

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

## Advanced Usage

### Generate Specific Category

```bash
python3 generate.py --category simple
```

### Generate Only Overview Plot

```bash
python3 generate.py --delta
```

### List All Categories

```bash
python3 generate.py --list
```

### Clean and Regenerate

```bash
python3 generate.py --clean
```

## Publication-Quality Benchmarks

For academic publication, use the enhanced benchmark setup with CPU isolation, NUMA pinning, and statistical analysis:

```bash
# 1. Setup system (requires root)
sudo bash scripts/setup_paper_benchmarks.sh

# 2. Run with statistics (multiple runs)
export BENCHMARK_RUNS=10  # Number of runs
bash scripts/run_paper_benchmarks.sh

# 3. Generate plots
python3 generate.py
```

This setup:
- Isolates CPUs from scheduler
- Pins to specific NUMA nodes
- Disables CPU frequency scaling
- Disables turbo boost
- Runs multiple iterations with statistical analysis
- Outputs mean, std dev, min, max for each measurement

See `PAPER_BENCHMARKS.md` for detailed documentation.

## Troubleshooting

### Build Issues

- **Missing libraries**: Install system packages or ensure `lib/lib/` contains `.so` files
- **pkg-config not found**: The build script falls back to `lib/lib/` if pkg-config fails

### Benchmark Issues

- **Category not found**: Check category name capitalization (e.g., `NonMatch` not `nomatch`)
- **Segmentation fault**: Some categories (like `scaling`) may crash on certain patterns - this is expected
- **No data in CSV**: Check that the benchmark executable ran successfully and produced output

### Plot Generation Issues

- **Missing columns**: Ensure CSV files have proper headers (may need to skip "Running category:" line)
- **Empty overview**: The script automatically merges `simple/` and `complex/` data if root-level files don't exist

## Notes

- All engines use anchored matching (`^pattern$`)
- PCRE2 uses JIT compilation for fair comparison
- std::regex uses `std::regex::optimize`
- Each measurement uses 10,000 iterations after warmup
- CTRE-SIMD shows 0ns instantiation time (compiles at C++ compile time)
- Category names are case-sensitive in the benchmark executables

