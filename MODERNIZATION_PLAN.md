# 🎨 C++20/23 Modernization Plan

## Goal: Modern C++ Features WITHOUT Performance Regression

---

## ✅ Safe Modernizations (Compile-Time Only)

### 1. **Add Concepts** (Type Safety, Zero Runtime Cost)
```cpp
// BEFORE:
template <typename Iterator, typename EndIterator>
inline constexpr bool has_at_least_bytes(Iterator current, EndIterator last, size_t n)

// AFTER:
template <std::random_access_iterator Iterator, std::sentinel_for<Iterator> EndIterator>
[[nodiscard]] inline constexpr bool has_at_least_bytes(Iterator current, EndIterator last, size_t n) noexcept
```

**Impact**: Better compile-time errors, zero runtime cost ✅

### 2. **Add [[nodiscard]]** (Safety, Zero Runtime Cost)
```cpp
[[nodiscard]] inline int get_simd_capability() noexcept { ... }
[[nodiscard]] inline bool has_avx2() noexcept { ... }
[[nodiscard]] inline constexpr bool has_at_least_bytes(...) noexcept { ... }
```

**Impact**: Catch bugs at compile-time ✅

### 3. **Use [[likely]] / [[unlikely]]** (Replace __builtin_expect)
```cpp
// BEFORE:
if (__builtin_expect(condition, 1)) { ... }

// AFTER (C++20):
if (condition) [[likely]] { ... }
```

**Impact**: Cleaner syntax, same optimization ✅

### 4. **constexpr Everything Possible** (Compile-Time Evaluation)
```cpp
// Pattern traits, size calculations, etc.
[[nodiscard]] constexpr bool is_single_char_pattern() noexcept { ... }
[[nodiscard]] constexpr size_t calculate_simd_threshold() noexcept { ... }
```

**Impact**: More compile-time evaluation ✅

### 5. **consteval for Compile-Time Only** (Enforce Compile-Time)
```cpp
// Things that MUST be compile-time
[[nodiscard]] consteval bool can_use_simd() noexcept { return CTRE_SIMD_ENABLED; }
```

**Impact**: Catches runtime usage errors ✅

### 6. **noexcept Specifications** (Better Optimization)
```cpp
// Add noexcept where we know functions don't throw
inline bool has_avx2() noexcept { ... }
inline int get_simd_capability() noexcept { ... }
```

**Impact**: Better optimization, clearer API ✅

---

## ⚠️ AVOID (Could Hurt Performance!)

### ❌ **DON'T Change Hot Path Logic**
- Keep SIMD intrinsics exactly as-is
- Don't mess with loop structures
- Preserve branch hints (even if converting to [[likely]])
- Keep fast paths unchanged

### ❌ **DON'T Add Unnecessary Abstractions**
- No wrapper classes for SIMD types
- No virtual functions
- No extra indirection

### ❌ **DON'T Force constexpr on Runtime Code**
- SIMD intrinsics can't be constexpr
- Runtime detection must stay runtime

---

## 📋 Modernization Checklist

### File: `simd_detection.hpp`
- [x] Add `[[nodiscard]]` to all query functions
- [x] Add `noexcept` to all functions
- [x] Use consteval for `can_use_simd()`
- [x] Modernize lambda initialization
- [ ] Keep backward compatibility (int constants)

### File: `simd_character_classes.hpp`
- [ ] Add `[[nodiscard]]` to match functions
- [ ] Replace `__builtin_expect` with `[[likely]]`
- [ ] Add `noexcept` where appropriate
- [ ] Add concepts for iterator types
- [ ] Improve comments with modern style
- [ ] **KEEP**: All SIMD intrinsic logic unchanged!

### File: `evaluation.hpp`
- [ ] Add `[[likely]]` / `[[unlikely]]` attributes
- [ ] Add `constexpr` to compile-time helpers
- [ ] Add concepts for template parameters
- [ ] **KEEP**: All dispatch logic unchanged!

### File: `simd_rose.hpp`
- [ ] Already modern (just created!)
- [ ] Add concepts
- [ ] Add `[[nodiscard]]`

---

## 🧪 Testing Strategy

### After Each Modernization:
1. **Compile test**: Does it compile?
2. **Unit test**: Does it work correctly?
3. **Performance test**: Quick benchmark (3 patterns)
4. **Full benchmark**: If quick test passes

### Acceptance Criteria:
- ✅ Compiles without warnings
- ✅ All tests pass
- ✅ Performance: 9.8-10.5x (within variance)
- ✅ No regressions > 5%

---

## 🎯 Expected Outcome

### Code Quality:
- ✅ Modern C++20/23 features
- ✅ Better type safety (concepts)
- ✅ Clearer intent ([[nodiscard]], [[likely]])
- ✅ More compile-time evaluation

### Performance:
- ✅ **SAME** or better (10.0-10.5x)
- ✅ No hot path changes
- ✅ Optimization hints preserved

---

## 🚀 Implementation Order

### Phase 1: Non-Invasive (Low Risk)
1. Add `[[nodiscard]]` attributes
2. Add `noexcept` specifications
3. Use consteval for compile-time functions

### Phase 2: Moderate Changes (Medium Risk)
4. Replace `__builtin_expect` with `[[likely]]`
5. Add concepts to template parameters
6. Add constexpr to helpers

### Phase 3: Verification
7. Run full benchmark
8. Verify 10.0-10.5x maintained
9. Document changes

---

**Key Principle**: **Modernize syntax, NOT logic!**

