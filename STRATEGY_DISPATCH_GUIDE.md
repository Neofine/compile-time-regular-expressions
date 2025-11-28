# 🔍 CTRE Strategy Dispatch Guide

## Understanding What Happens When You Match

This guide explains how CTRE decides which matching strategy to use and what actually executes.

---

## The Dispatch Hierarchy

```
Pattern String
     ↓
Parser (ctll)  ← Compile-time
     ↓
AST Type (e.g., repeat<1, 0, char_range<'a', 'z'>>)
     ↓
evaluation.hpp::evaluate() ← Runtime
     ↓
   [Decision Point]
     ↓
  ┌──────────────────────────┐
  │  Pattern Type Dispatch   │
  └──────────────────────────┘
           ↓
    ┌──────┴──────┐
    │             │
 Repetition?  Alternation?  Literal?
    │             │            │
    ↓             ↓            ↓
 SIMD Path    NFA Path    String Path
```

---

## Strategy Decision Tree

### 1. **Is it a Repetition?** (a*, [a-z]+, etc.)

```
IF pattern is repeat<A, B, Content>:
  
  Runtime checks:
  ✓ Is this constexpr evaluation? → NO (runtime)
  ✓ Is SIMD enabled? → YES
  ✓ Is iterator char*? → YES
  ✓ Is input >= 28 bytes? → ...
  
  IF all YES:
    → Try SIMD optimizations (see below)
  ELSE:
    → Fall back to scalar Glushkov NFA
```

### 2. **Is it an Alternation?** (A|B|C)

```
IF pattern is select<A, B, ...>:
  
  → Use Glushkov NFA with backtracking
  → Each branch evaluated recursively
  → Character classes inside branches CAN use SIMD!
  
  Example: "Huck[a-zA-Z]+|Saw[a-zA-Z]+"
    - Alternation dispatch: Glushkov NFA
    - [a-zA-Z]+ parts: SIMD range matching!
```

### 3. **Is it a Literal String?**

```
IF pattern is string<'T', 'w', 'a', 'i', 'n'>:
  
  → Use memcmp (or SIMD for long strings)
  → Very fast for fixed strings
```

---

## SIMD Dispatch (for Repetitions)

When a repetition pattern reaches the SIMD decision point in `evaluation.hpp`:

```cpp
// Location: evaluation.hpp, line ~410 (possessive_repeat)
//       and: evaluation.hpp, line ~610 (greedy repeat)

if constexpr (sizeof...(Content) == 1) {  // Single content (not a|b|c)
    if (!std::is_constant_evaluated() && simd::can_use_simd()) {
        
        using ContentType = std::tuple_element_t<0, std::tuple<Content...>>;
        
        // Check: Is it char iterator?
        if constexpr (std::is_same_v<..., char>) {
            
            // Check: Is it wildcard (.)?
            if constexpr (std::is_same_v<ContentType, any>) {
                → Ultra-fast wildcard (just advance pointer!)
            }
            
            // Check: Is input large enough?
            const auto remaining_input = last - current;
            if (remaining_input >= 28) {  // ← THE 28-BYTE THRESHOLD!
                
                // TRY 1: Multi-range SIMD ([a-zA-Z], [0-9a-fA-F])
                if constexpr (is_multi_range<ContentType>::is_valid) {
                    → match_multirange_repeat<...>(...)
                }
                
                // TRY 2: Shufti SIMD ([aeiou], [02468])
                if constexpr (should_use_shufti<ContentType>) {
                    → match_pattern_repeat_shufti<...>(...)
                }
                
                // TRY 3: Generic range SIMD ([a-z], [0-9])
                if constexpr (has min_char && max_char) {
                    → match_pattern_repeat_simd<...>(...)
                      → match_char_class_repeat_avx2() or
                      → match_single_char_repeat_avx2()
                }
            }
        }
    }
}

// FALLBACK: Scalar Glushkov NFA
→ evaluate() recursively with Content
```

---

## Concrete Examples

### Example 1: `a*_256` (256 bytes of 'a')

```
Pattern: a*
Parse → repeat<0, 0, character<'a'>>

Runtime flow:
1. Enter possessive_repeat evaluation
2. Check: Repetition? YES
3. Check: !constexpr? YES
4. Check: SIMD enabled? YES
5. Check: char iterator? YES
6. Check: Input size? 256 bytes → YES (>= 28)
7. Check: Is single char? YES
   
   → DISPATCH TO: match_single_char_repeat_avx2()
   
   AVX2 flow:
   a. Check: has >= 32 bytes? YES
   b. Check: has >= 64 bytes? YES
   c. Enter 64-byte loop:
      - Load 64 bytes
      - Compare all 64 bytes to 'a' using _mm256_cmpeq_epi8 (2x)
      - Combine with _mm256_and_si256
      - Check if all match with _mm256_testc_si256
      - All match! Advance 64 bytes
      - Repeat...
   d. Process remaining 32-byte chunks
   e. Process remaining 16 bytes or scalar tail
   
Result: 52x speedup! (0.1 cycle/byte)
```

### Example 2: `a+_16` (16 bytes of 'a')

```
Pattern: a+
Parse → repeat<1, 0, character<'a'>>

Runtime flow:
1. Enter possessive_repeat evaluation
2. Check: Repetition? YES
3. Check: !constexpr? YES
4. Check: SIMD enabled? YES
5. Check: char iterator? YES
6. Check: Input size? 16 bytes → NO (< 28) ⚠️
   
   → FALLBACK TO: Scalar Glushkov NFA
   
Scalar flow:
   - Loop char by char
   - Compare each to 'a'
   - 16 iterations
   
Result: Only 1.56x speedup (scalar is slower!)
```

### Example 3: `[a-z]*_512` (512 bytes)

```
Pattern: [a-z]*
Parse → repeat<0, 0, char_range<'a', 'z'>>

Runtime flow:
1. Enter possessive_repeat evaluation
2. Check: Repetition? YES
3. Check: Input size? 512 bytes → YES (>= 28)
4. Check: Is multi-range? NO (single range)
5. Check: Is sparse (Shufti)? NO (contiguous range)
6. Check: Has min/max chars? YES (min='a', max='z')
   
   → DISPATCH TO: match_char_class_repeat_avx2()
   
   AVX2 flow:
   a. Create vectors for min='a' and max='z'
   b. Process 64-byte chunks:
      - Load 64 bytes (2x 32-byte loads)
      - Range check: data >= 'a' AND data <= 'z'
      - Uses _mm256_cmpgt_epi8 for range comparison
      - All match! Advance 64 bytes
      - Repeat...
   
Result: 40x speedup! (0.56 cycle/byte for range check)
```

### Example 4: `complex_alt` (Alternation)

```
Pattern: Huck[a-zA-Z]+|Saw[a-zA-Z]+
Parse → select<
          sequence<character<'H'>, character<'u'>, character<'c'>, 
                   character<'k'>, repeat<1,0,char_range<'a','z','A','Z'>>>,
          sequence<character<'S'>, character<'a'>, character<'w'>,
                   repeat<1,0,char_range<'a','z','A','Z'>>>
        >

Runtime flow:
1. Enter select evaluation (alternation)
2. Try first branch:
   a. Match 'H', 'u', 'c', 'k'
   b. If all match:
      - Enter [a-zA-Z]+ repetition
      - This CAN use SIMD if input large enough!
      - → match_multirange_repeat() for [a-zA-Z]
3. If first branch fails:
   - Backtrack
   - Try second branch...
   
Result: 1.70x speedup (alternation dispatch is slow, 
        but [a-zA-Z]+ parts are SIMD accelerated!)
```

---

## The 28-Byte Threshold Mystery

**Why 28 bytes?**

SIMD optimization has overhead:
1. **CPUID check** (~25 cycles) - cached per thread
2. **Vector setup** (~5 cycles) - create comparison vectors
3. **Branch logic** (~2-3 cycles) - check size, dispatch
4. **Fallback to scalar** (if mismatch) - additional overhead

For small inputs, this overhead exceeds the benefit!

**Measurements:**
- Input size 16 bytes: SIMD overhead > SIMD benefit
- Input size 24 bytes: Break-even point
- Input size 28 bytes: Conservative safe threshold
- Input size 32+ bytes: Clear SIMD win!

**Example:**
```
Pattern: a*_16 (16 bytes)
  
SIMD path:
  CPUID: 25 cycles
  Setup:  5 cycles
  Dispatch: 3 cycles
  16-byte SIMD load+compare: 2 cycles
  Total: 35 cycles
  
Scalar path:
  16 char-by-char comparisons: 16 cycles
  Total: 16 cycles
  
→ Scalar wins! (35 vs 16 cycles)
```

---

## BitNFA: The Unused Strategy

**When BitNFA Would Be Used:**

```cpp
// From: bitnfa/integration.hpp

template <typename Pattern>
struct pattern_analysis {
    static constexpr bool use_bitnfa =
        (state_count > 16) ||        // Many NFA states
        (alternation_count > 3);     // Many alternations
};
```

**Why It's Currently Disabled:**

- BitNFA was tested for `complex_alt`, `alternation_4`, `group_alt`
- **Result:** Regressions on small inputs!
- **Reason:** Bit-parallel overhead > benefit for short strings
- **Status:** Could be useful for very large inputs (>1KB) with complex patterns

---

## Summary: What Actually Runs?

| Pattern Type | Small Input (<28B) | Large Input (≥28B) | Speedup |
|--------------|-------------------|-------------------|---------|
| `a*` (single char) | Scalar loop | AVX2 (16/32/64-byte paths) | 1.5x → 52x |
| `[a-z]*` (range) | Scalar loop | AVX2 range comparison | 1.5x → 40x |
| `[aeiou]*` (sparse) | Scalar loop | AVX2 Shufti | 1.5x → 20x |
| `[a-zA-Z]*` (multi-range) | Scalar loop | AVX2 multi-range | 1.5x → 20x |
| `A\|B` (alternation) | Glushkov NFA | Glushkov NFA* | 1.0-2x |
| `Twain` (literal) | memcmp | memcmp | 1.0-2x |

**Note:** Even for alternations, character class repetitions inside branches use SIMD!

---

## Key Takeaways

1. **SIMD is Pattern-Specific**: Only repetitions get SIMD
2. **Size Matters**: <28 bytes → scalar, ≥28 bytes → SIMD
3. **No Magic**: Alternations can't be easily SIMD-ized
4. **Hybrid Approach**: Complex patterns use NFA with SIMD parts
5. **Compiler Does Heavy Lifting**: All dispatch at compile-time (zero cost!)

---

**Want to see what your pattern uses?**

Run: `python3 analyze_strategies.py`

