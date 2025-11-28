# Hyperscan SIMD Techniques Analysis

## What We Already Have ✅

### 1. Shufti (Character Class Matching)
- **File**: `include/ctre/simd_shufti.hpp`
- **Purpose**: Fast matching of sparse character sets like `[aeiou]`
- **Status**: IMPLEMENTED ✅

### 2. Direct SIMD Comparison
- **Files**: `include/ctre/simd_character_classes.hpp`
- **Purpose**: Direct comparison for small character sets (1-6 chars)
- **Status**: IMPLEMENTED ✅
- **Finding**: We discovered direct comparison is 21x faster than Shufti for small sets!

### 3. Multi-range Matching
- **File**: `include/ctre/simd_multirange.hpp`  
- **Purpose**: Match multiple ranges like `[a-zA-Z0-9]`
- **Status**: IMPLEMENTED ✅

## Hyperscan Techniques We DON'T Have ❌

### 1. **Vermicelli** (Single Character Search)
- **Purpose**: Ultra-fast single character search using SIMD
- **Technique**: Uses `PCMPEQB` + `PMOVMSKB` to find character in 16 bytes at once
- **Key insight**: Can process 16 bytes with just 2 instructions!
- **Potential**: Could improve `a*`, `9*` patterns

### 2. **Double Vermicelli** (Two Character Pairs)
- **Purpose**: Fast search for two-character patterns like "ab"
- **Technique**: Search for both characters, then verify adjacency
- **Potential**: Could help literal patterns

### 3. **Noodle** (Short Literal String Search)
- **Purpose**: Fast search for short literal strings (2-8 bytes)
- **Technique**: Combines Vermicelli with mask checking
- **Potential**: Could improve literal patterns like "Twain", "ing"

### 4. **Streaming SIMD Extensions**
- **Purpose**: Process very long strings without reloading masks
- **Technique**: Keep SIMD registers "hot" across iterations
- **Potential**: Could improve long patterns (256+ bytes)

### 5. **Unaligned Load Optimization**
- **Purpose**: Handle unaligned data efficiently
- **Technique**: Use `_mm_loadu_si128` strategically
- **Status**: We use this, but could optimize further

## Most Promising for Us 🎯

### **#1: Vermicelli for Single Character**
**Why**: Our single-char patterns (a*, 9*) are still only 4-9x
**How**: Ultra-optimized single-character search
**Expected gain**: +20-30% for single-char patterns

### **#2: Noodle for Literals** 
**Why**: Literal patterns are 1.0x (no improvement)
**How**: Fast literal string search
**Expected gain**: Could get literals to 2-3x

### **#3: Streaming SIMD**
**Why**: Long patterns (256+ bytes) could be even faster
**How**: Keep SIMD state hot, reduce reload overhead
**Expected gain**: +5-10% for long patterns

## Implementation Strategy

### Phase 1: Vermicelli (Biggest Impact)
```cpp
// Ultra-fast single character search
template<char C>
const char* vermicelli_search(const char* begin, const char* end) {
    __m128i target = _mm_set1_epi8(C);
    
    while (end - begin >= 16) {
        __m128i data = _mm_loadu_si128((__m128i*)begin);
        __m128i cmp = _mm_cmpeq_epi8(data, target);
        int mask = _mm_movemask_epi8(cmp);
        
        if (mask) {
            return begin + __builtin_ctz(mask);
        }
        begin += 16;
    }
    
    // Scalar tail
    while (begin < end) {
        if (*begin == C) return begin;
        begin++;
    }
    return end;
}
```

### Phase 2: Noodle for Literals
```cpp
// Fast literal string search
const char* noodle_search(const char* begin, const char* end, 
                          const char* literal, size_t len) {
    if (len == 0) return begin;
    if (len == 1) return vermicelli_search(begin, end, literal[0]);
    
    // Search for first character, then verify rest
    const char* pos = begin;
    while (pos < end - len + 1) {
        pos = vermicelli_search(pos, end, literal[0]);
        if (pos == end) return end;
        
        if (memcmp(pos + 1, literal + 1, len - 1) == 0) {
            return pos;
        }
        pos++;
    }
    return end;
}
```

## Expected Impact on Our Patterns

### Single-Char Patterns (Vermicelli):
- a*_32: 4.5x → ~8x (+78%)
- 9*_32: 4.4x → ~8x (+82%)
- Impact: +0.3-0.4x average

### Literal Patterns (Noodle):
- literal_Twain: 1.0x → ~2.5x (+150%)
- char_literal_32: 1.33x → ~3x (+126%)
- Impact: +0.1-0.2x average

### Long Patterns (Streaming):
- Already at 40x+, diminishing returns
- Impact: +0.05x average

## Total Potential Gain
**Vermicelli**: +0.3-0.4x
**Noodle**: +0.1-0.2x
**Total**: +0.4-0.6x

**New estimate**: 9.7x + 0.5x = **10.2x** (still short of 11x but closer!)

## Next Steps
1. Implement Vermicelli for single-character search
2. Test on a*, 9*, z* patterns
3. If successful, implement Noodle for literals
4. Measure impact on full benchmark

