# SIMD Sequence Fusion - Implementation Plan

## 🎯 Goal
Match patterns like `[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}` (IPv4) 
in a single SIMD operation instead of 12+ scalar operations.

## 📋 Step-by-Step Plan

### Phase 1: Pattern Recognition (2-3 hours)

**Detect fusible sequences:**
```cpp
// In simd_pattern_analysis.hpp
template <typename... Elements>
struct sequence_fusion_trait {
    // Detect pattern: (charclass{N,M} literal)+ charclass{N,M}
    // Example: [0-9]{1,3}.[0-9]{1,3}.[0-9]{1,3}.[0-9]{1,3}
    
    static constexpr bool is_fusible = /* compile-time check */;
    static constexpr size_t min_length = /* sum of minimums */;
    static constexpr size_t max_length = /* sum of maximums */;
    static constexpr size_t num_segments = /* count segments */;
    
    // Extract structure: {class, min, max, literal}[]
    static constexpr auto segments = extract_segments();
};
```

**Recognition criteria:**
- All segments are character classes or literals
- Total length ≤ 32 bytes (fits in AVX2 register)
- At least 2 segments (otherwise use normal SIMD)

### Phase 2: SIMD Mask Generation (3-4 hours)

**Generate match masks:**
```cpp
// For IPv4: [0-9]{1,3}.[0-9]{1,3}.[0-9]{1,3}.[0-9]{1,3}
//
// Possible lengths: 7 ("1.1.1.1") to 15 ("192.168.255.255")
//
// Strategy: Try each possible length, generate mask:
//
// Length 7:  D D D D D D D  (D=digit, L=literal)
//            1 . 1 . 1 . 1
//
// Length 11: D D . D D . D D . D D
//            1 2 . 1 2 . 1 2 . 1 2
//
// Length 15: D D D . D D D . D D D . D D D
//            1 9 2 . 1 6 8 . 2 5 5 . 2 5 5

constexpr auto generate_ipv4_masks() {
    std::array<simd_mask, 9> masks; // lengths 7-15
    
    for (size_t len = 7; len <= 15; ++len) {
        masks[len-7] = {
            .digit_positions = /* which positions must be [0-9] */,
            .literal_positions = /* which positions must be '.' */,
            .literal_values = /* what values for literals */
        };
    }
    
    return masks;
}
```

### Phase 3: Runtime Matching (1-2 hours)

**Fast path for sequences:**
```cpp
// In evaluation.hpp
template <typename SequencePattern, typename... Tail>
constexpr auto evaluate(..., ctll::list<SequencePattern, Tail...>) {
    if constexpr (simd::sequence_fusion_trait<SequencePattern>::is_fusible) {
        constexpr auto trait = simd::sequence_fusion_trait<SequencePattern>{};
        
        // Fast path: fused sequence matching
        if (remaining >= trait.min_length && remaining <= trait.max_length) {
            auto result = simd::match_fused_sequence<SequencePattern>(current, last);
            if (result != current) {
                return evaluate(..., ctll::list<Tail...>);
            }
        }
    }
    
    // Fallback: normal evaluation
    return evaluate_normal(...);
}
```

**Actual SIMD matching:**
```cpp
// In simd_sequence_fusion.hpp
template <typename SequencePattern>
Iterator match_fused_sequence_avx2(Iterator begin, Iterator end) {
    constexpr auto trait = sequence_fusion_trait<SequencePattern>{};
    
    // Try each possible length
    for (size_t len = trait.min_length; len <= trait.max_length; ++len) {
        if (end - begin < len) continue;
        
        // Load input
        __m256i input = _mm256_loadu_si256((__m256i*)begin);
        
        // Check digit positions
        __m256i digit_mask = check_digits_at_positions(input, len);
        
        // Check literal positions  
        __m256i literal_mask = check_literals_at_positions(input, len);
        
        // Both must match
        __m256i combined = _mm256_and_si256(digit_mask, literal_mask);
        
        if (_mm256_movemask_epi8(combined) == expected_mask_for_length(len)) {
            return begin + len; // MATCH!
        }
    }
    
    return begin; // no match
}
```

## 📊 Expected Results

**IPv4 benchmark:**
```
Current (scalar):  26ns → 41ns with SIMD overhead = 0.64x
Proposed (fused):  26ns → 3-5ns = 5-8x speedup!
```

**Why faster:**
1. Single SIMD load instead of 12+ operations
2. Parallel checking of all positions
3. No loop overhead
4. Better branch prediction

## ⚖️ Complexity vs. Benefit

**Pros:**
✅ Fixes IPv4/MAC/UUID regressions completely
✅ 5-50x speedup on sequence patterns
✅ Compile-time detection (no runtime overhead)
✅ Generalizable to many patterns
✅ No binary bloat (only instantiate when detected)

**Cons:**
❌ 8-12 hours of work
❌ Complex mask generation logic
❌ Variable-length handling is tricky
❌ Limited to patterns ≤32 bytes

## 🎯 Recommendation

**YES, worth it!** Here's why:

1. **High impact**: Fixes the 3 worst regressions (IPv4, MAC, UUID)
2. **Clean solution**: Compile-time pattern fusion, no hacks
3. **Generalizable**: Works for dates, phone numbers, serial numbers, etc.
4. **Moderate effort**: 8-12 hours vs. potentially 10-50x speedup

**Next steps:**
1. Implement Phase 1 (pattern recognition) - 2-3 hours
2. Test with IPv4 pattern - 1 hour  
3. If successful, implement Phase 2 & 3 - 5-8 hours
4. Total: 8-12 hours for complete solution

Should I proceed with Phase 1?
