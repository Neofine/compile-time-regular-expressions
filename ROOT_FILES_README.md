# CTRE-SIMD Project Structure

Clean, organized repository for SIMD-accelerated compile-time regular expressions.

## 📂 Directory Structure

```
compile-time-regular-expressions/
├── include/ctre/              # Main CTRE-SIMD library (header-only)
│   ├── evaluation.hpp         # Core evaluation engine  
│   ├── simd_*.hpp            # SIMD optimization modules
│   ├── wrapper.hpp           # API entry points
│   └── ... (45 headers total)
├── plots/                     # Benchmarks & visualizations
│   ├── bench_real_world.cpp  # Real-world pattern benchmarks
│   ├── generate_*.py         # Plot generation scripts
│   ├── plot_*.png            # Generated plots (9 files)
│   └── README.md             # Complete plots documentation
├── thesis/                    # Thesis/paper LaTeX sources
│   ├── thesis_2_3_simd_implementation_final.tex  # Main chapter ⭐
│   ├── thesis_appendix_*.tex # Appendices
│   └── README.md             # Thesis documentation
├── tests/                     # Test suite
│   ├── individual_benchmarks/# Internal benchmark patterns
│   └── ...                   # Unit and integration tests
├── conanfile.py              # Conan package manager config
├── SEQUENCE_FUSION_PROPOSAL.md  # Next optimization plan ⭐
└── ROOT_FILES_README.md      # This file
```

## 📊 Quick Start

### Run Real-World Benchmarks
```bash
cd plots/
cat README.md  # Full documentation
```

### View Thesis Materials
```bash
cd thesis/
cat README.md  # Thesis structure guide
```

## 📚 Documentation

### `SEQUENCE_FUSION_PROPOSAL.md` ⭐ **Next Step**
Detailed plan for SIMD sequence matching optimization:
- **Goal**: Fix IPv4/MAC/UUID regressions via fused SIMD matching
- **Approach**: Match entire sequences like `[0-9]{1,3}\.[0-9]{1,3}\.` in 1-2 SIMD ops
- **Effort**: 8-12 hours implementation
- **Expected benefit**: 5-50x speedup on structured patterns
- **Bonus**: Generalizable to dates, phone numbers, serial numbers

### Directory READMEs
- `plots/README.md` - Complete benchmarking & visualization guide
- `thesis/README.md` - Thesis structure and writing guide

## 🎯 Current Performance

**Internal Benchmarks**: 9.7x average speedup (with sequence detection)

**Real-World Patterns**:
- ✅ **Large inputs (64B+)**: 10-50x faster than baseline CTRE
  - Hex strings: 12-33x
  - Alphanumeric: 10-14x  
  - Base64: 2-3x
  - Email: 3x

- ⚠️ **Structured/small patterns**: 0.2-0.76x (regressions)
  - IPv4 (11B): 0.64x
  - MAC (17B): 0.68x
  - UUID (36B): 0.76x
  - Small inputs (<16B): 0.2-0.6x

**Root Cause**: Binary bloat (2.5x larger) → I-cache pressure  
**Solution**: SIMD Sequence Fusion (see SEQUENCE_FUSION_PROPOSAL.md)

## 🏗️ SIMD Implementation

Current optimizations in `include/ctre/`:

### Core Modules
- `simd_detection.hpp` - Platform detection & thresholds
- `simd_character_classes.hpp` - Character class repetitions  
- `simd_shufti.hpp` - Sparse character sets (Shufti technique)
- `simd_multirange.hpp` - Multi-range patterns like `[a-zA-Z0-9]`
- `simd_pattern_analysis.hpp` - Compile-time pattern analysis
- `simd_string_matching.hpp` - Literal string search

### Configuration
- `SIMD_REPETITION_THRESHOLD = 32` bytes
- Sequence detection: Enabled (partial fix)
- Platform: AVX2 (primary), SSE4.2 (fallback), scalar (baseline)

## 🚀 Development Roadmap

### Phase 1: Sequence Pattern Recognition (2-3 hours)
- Detect fusible patterns at compile-time
- Extract segment structure and bounds
- Generate matching metadata

### Phase 2: SIMD Mask Generation (3-4 hours)
- Create position masks for variable-length segments
- Generate literal and character class checks
- Compile-time mask optimization

### Phase 3: Runtime Matching (1-2 hours)
- Implement fused sequence matcher
- Integrate into evaluation pipeline
- Validate correctness

### Phase 4: Validation & Benchmarking (2-3 hours)
- Re-run full benchmark suite
- Update plots and documentation
- Verify no new regressions

**Total**: 8-12 hours | **ROI**: Very high (fixes major regressions)

## 🔧 Development

**Compile flags**:
```bash
g++ -std=c++20 -O3 -march=native your_code.cpp
```

**Disable SIMD** (for comparison):
```bash
g++ -std=c++20 -O3 -DCTRE_DISABLE_SIMD your_code.cpp
```

**Quick benchmark**:
```bash
cd plots/
./bench_real_world | head -20
```

## 📖 Additional Resources

- **Implementation**: `include/ctre/` (45 header files)
- **Benchmarks**: `plots/` (real-world patterns)
- **Tests**: `tests/individual_benchmarks/` (internal patterns)
- **Thesis**: `thesis/` (LaTeX documentation)
- **Next Step**: `SEQUENCE_FUSION_PROPOSAL.md`

---

**Status**: ✅ Clean, organized, production-ready  
**Last Updated**: December 2025  
**Performance**: 9.7x average, targeting 15-20x with sequence fusion
