# 🔍 Teddy Algorithm Analysis for CTRE

## What is Teddy?

**Teddy** is a SIMD-based multi-substring search algorithm from Rust's `aho-corasick` and `regex-automata` crates. It's designed to find multiple literal strings simultaneously using SIMD shuffles.

### Key Characteristics:

1. **Purpose**: Fast multi-substring search (finding "foo" OR "bar" OR "baz" in text)
2. **Technique**: SIMD shuffles (SSSE3 `pshufb` / AVX2 `vpshufb`)
3. **Best for**: Alternations of literals (`"foo|bar|baz|qux"`)
4. **Not for**: Character classes, repetitions, or complex patterns

---

## How Teddy Works

### Core Algorithm:

```
1. Build "masks" from first 1-3 bytes of each literal
2. Use SIMD shuffles (pshufb) to check 16/32 bytes at once
3. Quick filter: Reject positions that can't match
4. Verification: Check full literals at candidate positions
```

### Example: `"foo|bar|baz"`

```
Step 1: Extract first bytes
  literals: ["foo", "bar", "baz"]
  first_bytes: ['f', 'b', 'b']
  
Step 2: Build lookup table (mask)
  mask['f'] = 0b001  (pattern 0)
  mask['b'] = 0b110  (patterns 1 and 2)
  
Step 3: SIMD scan (per 16 bytes)
  input: "hello bar world"
  positions: 0-15
  
  Use pshufb to look up all 16 positions at once:
    → Finds 'b' at position 6
    → mask['b'] = 0b110 (could be "bar" or "baz")
    
Step 4: Verify
  Check position 6-8: "bar" ✓ MATCH!
```

### Performance:

- **Scan speed**: ~0.5-1.0 GB/s (very fast!)
- **Best case**: 8-16 literals, 3-10 chars each
- **Overhead**: Small (~1-2KB for tables)

---

## Would Teddy Benefit CTRE?

### Let's Analyze Our Benchmark Patterns:

**Patterns with literal alternations:**
```
1. alternation_4: "Tom|Sawyer|Huckleberry|Finn"
   → Literals: 4 strings
   → Current: Glushkov NFA (1.00x)
   → With Teddy: Could be MUCH faster! 🔥

2. literal_Twain: "Twain"
   → Single literal (not alternation)
   → Current: memcmp (1.05x)
   → With Teddy: No benefit (overkill for 1 literal)
```

**Patterns with MIXED alternations:**
```
3. complex_alt: "Huck[a-zA-Z]+|Saw[a-zA-Z]+"
   → Literals: ["Huck", "Saw"] + character classes
   → Current: Glushkov NFA (1.77x)
   → With Teddy: Could scan for "Huck" or "Saw" first! 🔥
                 Then match [a-zA-Z]+ with SIMD!

4. group_alt: "([A-Za-z]awyer|[A-Za-z]inn)\\s"
   → Literals: ["awyer", "inn"] (after char class)
   → Current: Glushkov NFA (0.97x)
   → With Teddy: Could scan for "awyer" or "inn"! 🔥
```

**Other patterns (NOT applicable):**
```
• a*, [a-z]*, etc.: NOT literals → Teddy doesn't apply
• [aeiou]*: NOT literals → Teddy doesn't apply
• All 68 SIMD patterns: Already optimized → No benefit
```

### Summary:

| Pattern | Teddy Applicable? | Potential Benefit |
|---------|-------------------|-------------------|
| alternation_4 | ✅ **YES** | **High** (4 pure literals) |
| complex_alt | ⚠️ **Partial** | **Medium** (can scan for prefix) |
| group_alt | ⚠️ **Partial** | **Medium** (can scan for suffix) |
| Other 77 patterns | ❌ **NO** | None (not literal alternations) |

**Total applicable**: 1-3 patterns out of 80 (1-4%)

---

## Implementation Complexity

### What We'd Need to Implement:

1. **Literal Extraction**: Extract literals from alternations
   - ✅ We already have `literal_extraction_*.hpp` files!
   - ✅ Can extract from `select<string<...>, string<...>>`

2. **Teddy Masks**: Build lookup tables from first N bytes
   - ❌ Need to implement (new code)
   - Complexity: Medium (~200-300 lines)

3. **SIMD Shuffle**: Use `pshufb` / `vpshufb` for parallel lookup
   - ❌ Need to implement (new code)
   - Complexity: Medium-High (~300-500 lines)
   - Requires: SSSE3 (for pshufb)

4. **Verification**: Check full literals at candidate positions
   - ⚠️ Partially implemented (can use existing string matching)
   - Complexity: Low (~50-100 lines)

5. **Integration**: Hook into evaluation.hpp for alternations
   - ⚠️ Need dispatch logic
   - Complexity: Low-Medium (~100-150 lines)

**Total new code**: ~650-1050 lines
**Total complexity**: Medium-High
**Development time**: 1-2 weeks for full implementation

---

## Expected Performance Impact

### Theoretical Speedup:

**For `alternation_4` (Tom|Sawyer|Huckleberry|Finn):**

Current: Glushkov NFA
- Tries each branch sequentially
- ~4-8 comparisons per position
- Performance: 1.00x (baseline)

With Teddy:
- Scans 16 bytes at once with pshufb
- Quick filter rejects most positions
- Only verifies ~1-2 candidates per 100 bytes
- **Expected: 3-10x faster!** 🔥

**For `complex_alt` (Huck[a-zA-Z]+|Saw[a-zA-Z]+):**

Current: Glushkov NFA with SIMD char classes
- Tries "Huck" or "Saw" sequentially
- Performance: 1.77x

With Teddy:
- Scans for "Huck" or "Saw" with pshufb
- Then uses SIMD for [a-zA-Z]+
- **Expected: 2-5x faster!** 🔥

---

## Cost-Benefit Analysis

### Costs:

1. **Implementation**: 650-1050 lines of complex SIMD code
2. **Binary size**: +1-3KB for Teddy tables and code
3. **Maintenance**: New algorithm to maintain
4. **Complexity**: Adds another dispatch path
5. **Limited applicability**: Only 1-3 patterns (1-4%)

### Benefits:

1. **Performance**: 3-10x faster for literal alternations
2. **Benchmark improvement**: `alternation_4` goes from 1.00x → potentially 5-10x!
3. **Real-world value**: Literal alternations are common in practice
4. **Completeness**: Would match Rust regex capabilities

### Overall Assessment:

**Benefit/Cost Ratio**: **MEDIUM**

- ✅ **High benefit** for affected patterns (3-10x!)
- ⚠️ **Low coverage** (only 1-3 patterns out of 80)
- ❌ **High implementation cost** (1000+ lines)
- ⚠️ **Marginal average improvement** (~0.1-0.3x on average across all patterns)

---

## Comparison: Teddy vs BitNFA

We already have BitNFA for alternations. How does Teddy compare?

### Teddy:

- **Best for**: Pure literal alternations ("foo|bar|baz")
- **Performance**: 3-10x faster than sequential (very fast scan)
- **Limitation**: Only works for literals
- **Code**: +1000 lines

### BitNFA:

- **Best for**: Any alternations (literals, char classes, etc.)
- **Performance**: 10-19% faster than sequential
- **Limitation**: Not as fast as Teddy for pure literals
- **Code**: Already implemented (1555 lines)

### Hybrid Approach (Best of Both):

```
IF alternation has only literals:
  → Use Teddy (3-10x faster!)
ELSE IF alternation has char classes/complex:
  → Use BitNFA (10-19% faster)
ELSE:
  → Use Glushkov NFA (baseline)
```

---

## Alternative: Optimize Existing BitNFA

Instead of implementing Teddy, we could:

1. **Optimize BitNFA for literal alternations**
   - Add fast path for pure literals
   - Use SIMD string matching
   - Likely gain: 2-5x for literal alternations
   - Code: +100-200 lines

2. **Add literal prefix scanning to Glushkov NFA**
   - Extract common prefix/suffix
   - Scan for prefix with SIMD
   - Then match full pattern
   - Likely gain: 1.5-3x for patterns with literals
   - Code: +50-100 lines

---

## Our Benchmark Patterns: Detailed Analysis

### Pattern: alternation_4

```
Pattern: "Tom|Sawyer|Huckleberry|Finn"
Input: "Huckleberry"

Current (Glushkov NFA):
  Try "Tom": 'H' != 'T' → FAIL
  Try "Sawyer": 'H' != 'S' → FAIL
  Try "Huckleberry": 'H' == 'H', 'u' == 'u', ... → MATCH!
  Performance: 1.00x

With Teddy:
  Build mask: first_bytes = ['T', 'S', 'H', 'F']
  Scan input with pshufb:
    Position 0: 'H' → mask says "could be Huckleberry"
    Verify: "Huckleberry" == "Huckleberry" → MATCH!
  Performance: Est. 5-10x faster! 🔥
  
Speedup: Could go from 1.00x → 5-10x!
```

### Pattern: complex_alt

```
Pattern: "Huck[a-zA-Z]+|Saw[a-zA-Z]+"
Input: "Huckleberry"

Current (Glushkov NFA + SIMD):
  Try "Huck[a-zA-Z]+":
    Match "Huck": char by char
    Match [a-zA-Z]+: SIMD range check
  Performance: 1.77x

With Teddy + SIMD:
  Scan for "Huck" or "Saw" with pshufb:
    Found "Huck" at position 0
  Match [a-zA-Z]+ with SIMD: (same as current)
  Performance: Est. 2-5x faster! 🔥
  
Speedup: Could go from 1.77x → 3-8x!
```

### Overall Benchmark Impact:

```
Pattern         | Current | With Teddy | Improvement
----------------|---------|------------|------------
alternation_4   | 1.00x   | 5-10x      | +4-9x 🔥
complex_alt     | 1.77x   | 3-8x       | +1-6x 🔥
group_alt       | 0.97x   | 2-4x       | +1-3x 🔥
Other 77        | Avg     | No change  | 0x
----------------|---------|------------|------------
AVERAGE (80)    | 10.26x  | 10.5-11x   | +0.2-0.7x

Small average improvement, but BIG wins for literal alternations!
```

---

## Recommendation

### Option A: Implement Full Teddy ✅ (If you want completeness)

**Pros:**
- 3-10x faster for literal alternations!
- Matches Rust regex capabilities
- Best-in-class for this use case

**Cons:**
- 1000+ lines of complex code
- Only benefits 1-3 patterns (1-4%)
- Average improvement: +0.2-0.7x

**Verdict**: Worth it IF literal alternations are important to you

---

### Option B: Optimize BitNFA for Literals ⚠️ (Simpler)

**Pros:**
- 2-5x faster for literal alternations
- Only +100-200 lines
- Leverages existing BitNFA code

**Cons:**
- Not as fast as pure Teddy
- Still only benefits 1-3 patterns

**Verdict**: Good compromise between effort and benefit

---

### Option C: Do Nothing ❌ (Focus elsewhere)

**Pros:**
- No implementation cost
- No additional complexity

**Cons:**
- Literal alternations remain slow (1.00x)
- Misses opportunity for 5-10x improvement

**Verdict**: Not recommended - easy wins available!

---

## My Recommendation: **Option B (Optimize BitNFA)**

**Reasoning:**

1. **Better ROI**: +100-200 lines for 2-5x improvement vs +1000 lines for 3-10x
2. **Leverage existing**: Build on BitNFA we already have
3. **Simpler**: Less complex than full Teddy implementation
4. **Good enough**: 2-5x is still excellent for the affected patterns
5. **Incremental**: Can add full Teddy later if needed

**Implementation plan:**
1. Add literal extraction for select<string<...>, string<...>>
2. Add fast literal scanning to BitNFA
3. Use SIMD memcmp for verification
4. Expected: 2-5x for `alternation_4`, 1.5-3x for `complex_alt`

---

## Next Steps (If You Want to Proceed)

### Quick Prototype (Option B):

1. Extract literals from alternation patterns
2. Add SIMD literal scanning to BitNFA
3. Benchmark on `alternation_4` and `complex_alt`
4. If successful, integrate fully

**Estimated time**: 2-4 hours for prototype
**Expected gain**: 2-5x for literal alternations

---

## Conclusion

**Teddy would help**, but:
- ✅ Only 1-3 patterns benefit (1-4% of benchmarks)
- ✅ Could give 3-10x speedup for those patterns!
- ❌ 1000+ lines of complex code
- ⚠️ Average improvement: only +0.2-0.7x

**Better approach**: Optimize BitNFA for literals first (simpler, 80% of benefit)

**Want me to implement the BitNFA optimization?** I can show you how to get 2-5x on `alternation_4` with just 100-200 lines! 🚀

