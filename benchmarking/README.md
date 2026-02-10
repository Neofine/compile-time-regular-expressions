# Benchmarks

Compare CTRE-SIMD against CTRE (original), CTRE-Scalar, RE2, PCRE2, Hyperscan, and std::regex.

## Requirements

```bash
sudo apt install libre2-dev libpcre2-dev libhyperscan-dev
pip install pandas matplotlib numpy  # For analysis
```

## Usage

```bash
./bench.sh --build              # Build only
./bench.sh Simple               # Run Simple category  
./bench.sh --build Simple       # Build then run
./bench.sh > results.csv        # Run all, save to CSV
sudo ./bench.sh --perf Simple   # Run with CPU tuning
```

## Analysis

```bash
./bench.sh Simple > results.csv
python3 analyze.py results.csv                    # Full analysis + plots
python3 analyze.py results.csv --no-plots         # CLI tables only
python3 analyze.py results.csv -o figures/        # Custom output dir
```

**Generated plots:**
- `comparison.png` — Bar chart comparing all 7 engines
- `speedup.png` — CTRE-SIMD speedup vs each engine
- `heatmap.png` — Speedup grid (pattern × input size)
- `scaling.png` — Performance scaling curves

## Engines Compared

| Engine | Description |
|--------|-------------|
| CTRE-SIMD | This library (SIMD-optimized) |
| CTRE-Scalar | This library with SIMD disabled |
| CTRE | Original upstream CTRE (baseline) |
| RE2 | Google's RE2 |
| PCRE2 | Perl Compatible Regular Expressions |
| Hyperscan | Intel's high-performance regex |
| std::regex | C++ standard library |

## Categories

| Category | Description |
|----------|-------------|
| Simple | `[0-9]+`, `[a-z]+`, `[aeiou]+` |
| Complex | Decimal, hex, identifiers, URLs |
| RealWorld | IPv4, UUID, email, dates |
| Scaling | Alternation vs character class |
| NonMatch | Prefilter rejection performance |
| Small | 1-16 byte inputs |
| Large | 32KB-8MB inputs |
| Fallback | Backrefs, lazy quantifiers |
| Adversarial | Edge cases, worst cases |
| Instantiation | Pattern compilation overhead |

## Output Format

CSV: `Pattern,Engine,Input_Size,Time_ns,Matches`

- **Time_ns** — Execution time in nanoseconds (ns)
- **Input_Size** — Input string length in bytes
- **Matches** — Number of successful matches (for verification)

## Options

| Flag | Description |
|------|-------------|
| `-b, --build` | Build benchmark executables |
| `-p, --perf` | Performance mode (disables ASLR, sets CPU governor) |
| `-h, --help` | Show help |
