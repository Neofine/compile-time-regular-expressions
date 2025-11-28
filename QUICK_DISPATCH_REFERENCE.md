# ⚡ Quick Dispatch Reference

## Want to know which strategy a pattern uses? Here's how:

---

## Option 1: Run the Analyzer (Fastest!)

```bash
python3 analyze_strategies.py
```

Shows all 80 patterns grouped by strategy type!

---

## Option 2: Manual Pattern Analysis

### Is it a **Repetition**? (a*, [a-z]+, etc.)

```
✅ CAN use SIMD!

Check input size:
  • Input < 28 bytes → Scalar Glushkov NFA (overhead > benefit)
  • Input ≥ 28 bytes → SIMD path!

SIMD sub-dispatch:
  • Single char (a*, z+) → match_single_char_repeat_avx2()
  • Range ([a-z]*) → match_char_class_repeat_avx2()
  • Sparse ([aeiou]*) → match_pattern_repeat_shufti()
  • Multi-range ([a-zA-Z]*) → match_multirange_repeat()
```

### Is it an **Alternation**? (A|B|C)

```
❌ Can't SIMD the alternation dispatch!

Uses: Glushkov NFA with backtracking
  • Tries each branch sequentially
  • BUT: Character classes inside branches CAN use SIMD!

Example: "Huck[a-zA-Z]+|Saw[a-zA-Z]+"
  • Alternation dispatch: Scalar (char by char)
  • [a-zA-Z]+ parts: SIMD if long enough!
```

### Is it a **Literal String**? (Twain)

```
Uses: memcmp (or SIMD for very long strings)
Performance: 1.0-1.5x (memcmp is already fast!)
```

### Is it **Complex**? (Sequences, groups, etc.)

```
Uses: General Glushkov NFA
Performance: 1.0-1.5x (unavoidable overhead)
```

---

## Quick Lookup Table

| Pattern | Input Size | Strategy | Speedup |
|---------|------------|----------|---------|
| `a*` | <28B | Scalar | 1.5x |
| `a*` | 32B | SIMD (32-byte path) | 16x |
| `a*` | 64B+ | SIMD (64-byte loop) | 24-52x |
| `[a-z]*` | <28B | Scalar | 1.2x |
| `[a-z]*` | 32B+ | SIMD (range check) | 10-40x |
| `[aeiou]*` | 32B+ | SIMD (Shufti) | 15-25x |
| `A\|B` | Any | Glushkov NFA | 1.0-2x |
| `Literal` | Any | memcmp | 1.0-1.5x |

---

## The 28-Byte Threshold Rule

**Simple rule of thumb:**

```
IF pattern is repetition (*, +):
    IF input >= 28 bytes:
        → Uses SIMD (fast!)
    ELSE:
        → Uses scalar (SIMD overhead not worth it)
ELSE (alternation, complex):
    → Uses Glushkov NFA (can't easily SIMD)
```

---

## Fast Path Decision Tree

For repetition patterns with input >= 28 bytes:

```
Input size?
  ├─ 16-31 bytes → 16-byte SSE4.2 fast path
  ├─ 32-63 bytes → 32-byte AVX2 fast path
  └─ 64+ bytes   → 64-byte AVX2 unrolled loop (BEST!)
```

---

## Code Locations

**Main dispatch:**
- `include/ctre/evaluation.hpp` lines ~410 and ~610

**SIMD implementations:**
- `include/ctre/simd_character_classes.hpp`
  - Line ~740: `match_single_char_repeat_avx2()`
  - Line ~390: `match_char_class_repeat_avx2()`

**Pattern analysis:**
- `include/ctre/simd_character_classes.hpp` lines ~76-200
  - Trait detection for SIMD eligibility

---

## Debugging a Pattern

### Step 1: Identify Pattern Type

Run: `grep "PATTERN_NAME" tests/master_benchmark.cpp`

### Step 2: Check Pattern Structure

- Has `*` or `+`? → Repetition (might use SIMD)
- Has `|`? → Alternation (uses NFA)
- Just letters? → Literal (uses memcmp)

### Step 3: Check Input Size

- Look at pattern name (e.g., `a*_16` = 16 bytes)
- Is it >= 28 bytes?
  - YES → Should use SIMD
  - NO → Will use scalar

### Step 4: Predict Performance

```
Repetition + large input (≥64B) → Expect 20-50x
Repetition + medium input (32-63B) → Expect 10-20x
Repetition + small input (<28B) → Expect 1.5-5x
Alternation → Expect 1.0-2x
Literal/Complex → Expect 1.0-1.5x
```

---

## Why Your Patterns Perform As They Do

### `a+_16` (1.56x - Below threshold!)
- Pattern: Repetition ✅
- Input: 16 bytes ❌ (< 28)
- **Strategy:** Scalar fallback
- **Why:** SIMD overhead > benefit for 16 bytes

### `a+_256` (52x - Optimal!)
- Pattern: Repetition ✅
- Input: 256 bytes ✅
- **Strategy:** 64-byte AVX2 loop (4 iterations)
- **Why:** Amortizes overhead, perfect SIMD utilization!

### `complex_alt` (1.7x - Alternation!)
- Pattern: Alternation ❌
- **Strategy:** Glushkov NFA with backtracking
- **Why:** Can't SIMD the alternation dispatch logic
- **Note:** [a-zA-Z]+ parts inside DO use SIMD if long enough!

### `negated_class` (0.99x - Complex!)
- Pattern: Sequence `[a-q][^u-z]{13}x` ❌
- **Strategy:** Mixed (not pure repetition)
- **Why:** Each part handled separately, negated class is harder

---

## Key Takeaway

**CTRE's dispatch is SMART and AUTOMATIC:**
- ✅ Uses SIMD where it helps (85% of patterns!)
- ✅ Avoids SIMD where it hurts (<28 bytes)
- ✅ Falls back to NFA for complex patterns
- ✅ All decisions at compile-time (zero cost!)

**The 28-byte threshold is the key to understanding everything!**

---

**Want more details?**
- See `DISPATCH_FLOW_VISUAL.md` for complete execution traces
- See `STRATEGY_DISPATCH_GUIDE.md` for full dispatch logic
- Run `python3 analyze_strategies.py` for your patterns
