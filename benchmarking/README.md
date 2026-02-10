# Benchmarks

Compare CTRE-SIMD against CTRE (original), CTRE-Scalar, RE2, PCRE2, Hyperscan, and std::regex.

## Requirements

```bash
sudo apt install libre2-dev libpcre2-dev libhyperscan-dev
pip install -r requirements.txt   # pandas, matplotlib, numpy, seaborn
```

Plots use a built-in blog-style (clean spines, light gray plot area, 300 DPI).

### Alternative: Plotly (different look)

For a different visual style (web-style, rounded bars, soft grid), use the Plotly backend:

```bash
pip install plotly kaleido pandas
python3 analyze_plotly.py results.csv -o plots_plotly
```

Same CSV and same chart types; output goes to `plots_plotly/` so you can compare with `plots/`.

### Other Python viz options

| Library | Best for | Look |
|--------|----------|------|
| **Plotly** | Interactive + static PNG (Kaleido) | Polished web-style, rounded, distinct |
| **Altair** | Declarative, Jupyter, save PNG (vl-convert) | Clean, minimal, Vega-Lite |
| **ProPlot** | Same code as matplotlib, nicer defaults | Drop-in; better colors/layout/fonts |
| **Lets-Plot** | Grammar-of-graphics (ggplot2-style) | Themes (e.g. high-contrast), different palette |

If you want “not ordinary” without changing code, try **ProPlot** (`pip install proplot` then `import proplot as pplt` and use `pplt.subplots()` instead of `plt.subplots()`). For a clearly different look from matplotlib, use **analyze_plotly.py** or explore **Altair** for a more minimal style.

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
