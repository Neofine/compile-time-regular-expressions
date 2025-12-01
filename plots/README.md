# CTRE-SIMD Performance Plots

This directory contains all benchmark programs, plot generation scripts, and generated visualizations.

## 📊 Benchmark Programs

### `bench_real_world.cpp`
Benchmarks real-world regex patterns against multiple engines:
- **Patterns**: IPv4, MAC, UUID, hex, alphanumeric, base64, email
- **Engines**: CTRE-SIMD, CTRE (baseline), std::regex, PCRE2, Hyperscan, RE2
- **Input sizes**: 8B, 16B, 32B, 64B, 128B, 256B

**Compile & Run**:
```bash
g++ -std=c++20 -O3 -march=native bench_real_world.cpp -o bench_real_world
./bench_real_world > results.csv
```

### `bench_real_world_ctre.cpp`
Baseline CTRE benchmark (SIMD disabled for comparison):
```bash
g++ -std=c++20 -O3 -DCTRE_DISABLE_SIMD bench_real_world_ctre.cpp -o bench_real_world_ctre
./bench_real_world_ctre > ctre_baseline.csv
```

## 🐍 Plot Generation Scripts

### `generate_complete_plots.py`
Generates individual pattern plots and comprehensive grids:
- Reads CSV output from benchmarks
- Creates time vs. input size plots
- Generates throughput comparisons
- Produces multi-engine grid visualization

**Usage**:
```bash
python3 generate_complete_plots.py
```

**Outputs**:
- `plot_all_engines_grid.png` - Grid of all patterns × engines
- `plot_ctre_simd_speedup_all_patterns.png` - Speedup overview
- `plot_realworld_*.png` - Individual pattern analyses

### `generate_scaling_plots.py`
Generates scaling analysis plots (legacy/alternative visualization):
```bash
python3 generate_scaling_plots.py
```

## 📈 Generated Plots

### Comprehensive Views
- **`plot_all_engines_grid.png`**  
  Grid comparison of all engines across all patterns
  
- **`plot_ctre_simd_speedup_all_patterns.png`**  
  CTRE-SIMD speedup relative to baseline CTRE

### Real-World Pattern Analysis
Each plot shows:
- **Left panel**: Processing time (lower is better)
- **Right panel**: Throughput in MB/s (higher is better)
- **X-axis**: Input size (log scale)
- **All engines**: CTRE-SIMD vs competitors

Patterns:
- `plot_realworld_ipv4_updated.png` - IPv4 address matching
- `plot_realworld_mac_updated.png` - MAC address matching
- `plot_realworld_uuid_updated.png` - UUID matching
- `plot_realworld_hex_updated.png` - Hexadecimal strings
- `plot_realworld_alnum_updated.png` - Alphanumeric strings
- `plot_realworld_base64_updated.png` - Base64 encoding
- `plot_realworld_email_updated.png` - Email addresses

## 🔄 Regenerating Plots

To regenerate all plots from scratch:

```bash
# 1. Run benchmarks (takes ~2 minutes)
./bench_real_world > real_world_simd.csv
./bench_real_world_ctre > real_world_ctre.csv

# 2. Combine results
cat <(echo "Pattern,Engine,Input_Size,Time_ns,Description") \
    <(cat real_world_simd.csv real_world_ctre.csv | grep -v "^Pattern,Engine") \
    > combined_results.csv

# 3. Generate plots
python3 generate_complete_plots.py

# 4. View results
ls -lh plot_*.png
```

## 📊 Current Performance Summary

**CTRE-SIMD vs Original CTRE:**

✅ **Big Wins** (64B+ inputs):
- Hex strings: **12-33x faster**
- Alphanumeric: **10-14x faster**
- Base64: **2-3x faster**
- Email: **3x faster**

⚠️ **Regressions** (small/structured patterns):
- IPv4 (11B): **0.64x** (slower due to binary bloat)
- MAC (17B): **0.68x**
- UUID (36B): **0.76x**
- Small inputs (<16B): **0.2-0.6x**

**Root Cause**: Binary bloat from SIMD code (2.5x larger) causes I-cache pressure, affecting all patterns.

**Solution In Progress**: SIMD Sequence Fusion (see ../SEQUENCE_FUSION_PROPOSAL.md)

## 📝 Notes

- All benchmarks use `-O3 -march=native` for maximum performance
- Results may vary based on CPU architecture
- Benchmarks use `ctre::match()` for anchored matching from start
- Real-world patterns use typical input sizes for each pattern type
