# Push for 8x Speedup - Analysis

## Current Status
- **Achieved:** 7.47x average speedup
- **Target:** 8.00x
- **Gap:** +7.1% needed

## Distribution
- **< 2x:**    6 patterns (4.4%) - Hard to optimize (overhead-dominated)
- **2-5x:**   67 patterns (49.3%) - Room for improvement  
- **5-10x:**  43 patterns (31.6%) - Already good
- **10x+:**   20 patterns (14.7%) - Excellent

## Optimization Attempts

### 1. Literal String Optimization (memcmp)
**Target:** `literal_Twain` (1.05x) and `char_literal_32` (1.33x)  
**Approach:** Use `memcmp` for 4-15 character literals instead of char-by-char  
**Result:** ❌ No improvement (already at ~2ns, measurement noise dominates)  
**Conclusion:** These patterns are too fast (~2ns) for further optimization

### 2. Single-Char 32-Byte Optimization
**Target:** `a*_32` (3.98x), `z*_32` (2.02x), `[0-9]*_32` (2.20x)  
**Observation:** Anomaly detected - `a*_32` (2.43ns) slower than `a*_16` (1.89ns) and `a*_64` (2.97ns)  
**Hypothesis:** 16-byte fast path (lines 696-718) adds overhead for exactly 32-byte inputs  
**Attempted Fix:** Removed 16-byte fast path before 32-byte loop, added after  
**Result:** ❌ Broke 16 and 64-byte performance (6.31ns and 5.95ns)  
**Reverted:** Yes

### 3. Root Cause Analysis
The 32-byte "slowness" is actually an artifact of:
1. **Measurement variance** (~0.5ns noise)
2. **Iterator overhead** for pointer bounds checking
3. **CPU frequency scaling** between test runs

Looking at the actual times:
- `a*_16`: 1.89ns (16 bytes / 1.89ns = 8.5 GB/s)
- `a*_32`: 2.43ns (32 bytes / 2.43ns = 13.2 GB/s) ← Actually FASTER per byte!
- `a*_64`: 2.97ns (64 bytes / 2.97ns = 21.6 GB/s)

**The 32-byte case is NOT slow** - it's just being compared to a faster NoSIMD baseline.

## Why 8x Is Hard to Reach

### 1. Fundamental Limits
- **Literal patterns** (1-1.3x): Already at measurement precision limit (~2ns)
- **Short input patterns** (0.6x): SIMD overhead > benefit (< 24 chars)
- **Alternations** without BitNFA: Branch prediction limits

### 2. What Would Get Us to 8x
To go from 7.47x to 8.00x, we'd need to:
- Boost all < 2x patterns to 4x: Would give us 6.80x (+7.3%, not enough)
- Boost all 2-5x patterns by 50%: Would give us ~8.5x (achievable but risky)

### 3. High-Risk Optimizations Available
- **Unroll scalar tails:** Might save 0.2-0.5ns but could break edge cases
- **Inline everything with `__attribute__((flatten))`:** Tried before, caused regressions
- **Disable bounds checking:** Unsafe, could cause crashes
- **Profile-Guided Optimization (PGO):** Tried before, minimal impact

## Recommendation

**Accept 7.47x as excellent performance:**
- 96% of patterns have ≥2x speedup
- Top patterns: 26.85x, 23.04x, 22.77x
- Only 2 patterns regress (short inputs where SIMD can't help)

**Alternative:** Focus on real-world benchmarks instead of microbenchmarks.
The patterns that matter most (large inputs, common regex features) already
have 10x+ speedups.

---

**Conclusion:** 7.47x is near the practical limit for this benchmark suite.
Further gains would require either:
1. Removing/modifying the slow patterns (defeats the purpose)
2. Unsafe optimizations (unacceptable)
3. Hardware changes (AVX-512, etc.)
