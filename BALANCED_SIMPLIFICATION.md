# 🎯 Balanced Simplification Plan

## Goal: Simplify Code While Keeping ALL Patterns Performing Well

**Current Baseline:** 10.22x average (balanced for all pattern sizes)

---

## Micro-Optimizations to Test (In Order)

### 1. **Remove 32-byte Fast Path** (Test First!)

**Current Code:**
```cpp
// PERF: 32-byte fast path for inputs between 32-63 bytes
if (has_at_least_bytes(current, last, 32) && !has_at_least_bytes(current, last, 64)) {
    // Special handling for 32-byte inputs
    ...
}
```

**Hypothesis:** The 64-byte loop + 32-byte tail loop might handle this fine!

**Risk:** Might help 32-byte patterns specifically

---

### 2. **Remove Early Returns After Exact Matches**

**Current Code:**
```cpp
if (current >= last) [[likely]] {
    return current;
}
```

**Hypothesis:** The loop condition will catch this anyway!

**Risk:** Low - probably just adds branches

---

### 3. **Simplify Combined testc in 64-byte Loop**

**Current Code:**
```cpp
__m256i combined = _mm256_and_si256(result1, result2);
if (_mm256_testc_si256(combined, all_ones)) [[likely]] {
    // Both match
} else {
    if (_mm256_testc_si256(result1, all_ones)) {
        // Check which chunk failed
    }
}
```

**Alternative:**
```cpp
if (_mm256_testc_si256(result1, all_ones) && 
    _mm256_testc_si256(result2, all_ones)) {
    // Both match
}
```

**Hypothesis:** Might be simpler and compiler can optimize!

**Risk:** Medium - this was supposedly optimized for ILP

---

### 4. **Remove Some Branch Hints**

**Current:** Many `[[likely]]` and `[[unlikely]]` attributes

**Hypothesis:** Compiler might profile better than our guesses!

**Risk:** Low - compiler is smart

---

### 5. **Test if We Need testc vs movemask**

**Current:**
```cpp
if (_mm256_testc_si256(result, all_ones)) { ... }
```

**Alternative:**
```cpp
int mask = _mm256_movemask_epi8(result);
if (mask == -1) { ... }
```

**Hypothesis:** Might be simpler!

**Risk:** Medium - testc was supposedly faster

---

## Testing Methodology

### For Each Simplification:

1. **Make the change**
2. **Test critical patterns:**
   - negated_class (must stay ~1.0x)
   - complex_alt (must stay ~1.7x)
   - a*_16 (must stay ~5x)
   - a+_16 (must stay ~1.5x)
3. **Test overall average** (must stay 9.8-10.5x)
4. **If all good:** Keep it and move to next!
5. **If regression > 5%:** Revert and document why needed

---

## Success Criteria

✅ **Keep simplification if:**
- All priority patterns stay within 5% of baseline
- Overall average stays 9.8-10.5x
- Code is noticeably simpler

❌ **Revert if:**
- Any priority pattern drops > 5%
- Overall average drops below 9.8x
- No significant simplification

---

## Expected Outcome

**Best Case:**
- Remove 20-30% of micro-optimizations
- Keep 10.0-10.3x balanced performance
- Much cleaner, simpler code

**Realistic Case:**
- Remove 10-15% of micro-optimizations
- Keep 9.9-10.2x performance
- Somewhat simpler code

**Worst Case:**
- Find that most micro-opts are needed
- Keep current balance
- Document why each is necessary

