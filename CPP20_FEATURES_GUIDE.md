# C++20 Features Quick Reference Guide

## For CTRE Developers and Contributors

This guide explains the modern C++20 features used in CTRE and how to use them correctly.

---

## 1. [[nodiscard]] Attribute

### What It Does
Warns if a function's return value is ignored.

### When to Use
✅ Functions that return important values (errors, results, queries)
❌ Functions that are called for side effects (logging, mutations)

### Examples

```cpp
// ✅ GOOD: Query function should use [[nodiscard]]
[[nodiscard]] inline bool has_avx2() noexcept {
    // ...
}

// Usage:
if (has_avx2()) { ... }  // ✅ OK
has_avx2();               // ⚠️ WARNING: ignoring return value!

// ✅ GOOD: Functions returning values
[[nodiscard]] inline int get_simd_capability() noexcept { ... }
[[nodiscard]] consteval bool can_use_simd() noexcept { ... }

// ❌ BAD: Don't use on void functions
[[nodiscard]] void process_data() { ... }  // Compile error!
```

---

## 2. consteval - Compile-Time Only

### What It Does
Forces a function to be evaluated at compile-time ONLY.

### When to Use
✅ Functions that query compile-time constants
✅ Configuration checks
❌ Anything that needs runtime data

### Examples

```cpp
// ✅ GOOD: Compile-time configuration query
[[nodiscard]] consteval bool can_use_simd() noexcept {
    return CTRE_SIMD_ENABLED;  // Macro, always compile-time
}

// Usage:
constexpr bool simd_enabled = can_use_simd();  // ✅ OK
bool enabled = can_use_simd();                  // ✅ OK (still compile-time!)
bool enabled = can_use_simd(runtime_value);    // ❌ COMPILE ERROR!

// ❌ BAD: Can't use with runtime data
consteval int get_value(int x) {
    return x * 2;
}
int result = get_value(42);        // ✅ OK (literal)
int result = get_value(user_input()); // ❌ COMPILE ERROR!
```

### consteval vs constexpr

| Feature | consteval | constexpr |
|---------|-----------|-----------|
| Compile-time? | **ALWAYS** | Optional |
| Runtime? | **NEVER** | Allowed |
| Use for configs | ✅ Perfect | ⚠️ OK but weaker |
| Use with runtime data | ❌ Never | ✅ Yes |

---

## 3. [[likely]] / [[unlikely]] - Branch Hints

### What They Do
Tell the compiler which branch is more common for better optimization.

### When to Use
✅ Hot paths (very common)
✅ Cold paths (error handling, rare cases)
❌ Don't guess! Profile first!

### Examples

```cpp
// ✅ GOOD: Hot path optimization
if (_mm256_testc_si256(result, all_ones)) [[likely]] {
    // Hot path: all bytes match (common for long runs)
    current += 32;
    count += 32;
} else {
    // Cold path: mismatch found (less common)
    int mask = _mm256_movemask_epi8(result);
    int first_mismatch = __builtin_ctz(~mask);
    return current + first_mismatch;
}

// ✅ GOOD: Rare condition optimization
if (!has_at_least_bytes(current, last, 64)) [[unlikely]] {
    // Unlikely: most inputs are >= 64 bytes
    break;
}

// ❌ BAD: Don't use when unsure!
if (user_preference) [[likely]] {  // Don't guess!
    // ...
}
```

### Replaces __builtin_expect

```cpp
// OLD WAY (GCC/Clang specific):
if (__builtin_expect(condition, 1)) { ... }  // Likely
if (__builtin_expect(!condition, 0)) { ... } // Unlikely

// NEW WAY (C++20 standard):
if (condition) [[likely]] { ... }
if (condition) [[unlikely]] { ... }
```

**Benefits:**
- ✅ Standard C++20 (portable!)
- ✅ More readable
- ✅ Same performance

---

## 4. noexcept - Exception Safety

### What It Does
Declares that a function never throws exceptions.

### When to Use
✅ Performance-critical functions
✅ Functions that truly can't throw
❌ Functions that might throw

### Examples

```cpp
// ✅ GOOD: SIMD operations don't throw
[[nodiscard]] inline bool has_avx2() noexcept {
    // CPUID intrinsics don't throw
    // ...
}

// ✅ GOOD: Compile-time functions don't throw
[[nodiscard]] consteval bool can_use_simd() noexcept {
    return CTRE_SIMD_ENABLED;
}

// ❌ BAD: Function might throw
int parse_int(const std::string& s) noexcept {
    return std::stoi(s);  // Can throw! Violates noexcept!
}
```

### Benefits
- ✅ Better optimization (compiler knows no cleanup needed)
- ✅ Clearer API contracts
- ✅ std::move_if_noexcept uses this
- ✅ Better performance in STL containers

---

## 5. C++20 Concepts

### What They Do
Constrain template parameters with named, reusable requirements.

### When to Use
✅ Template functions/classes
✅ Type constraints
✅ Better error messages

### Examples

```cpp
// BEFORE: Verbose requires clause
template <typename Iterator, typename EndIterator>
[[nodiscard]] inline constexpr bool has_at_least_bytes(
    Iterator current, EndIterator last, std::size_t n) noexcept
    requires requires { 
        { *current } -> std::convertible_to<char>;
        { ++current } -> std::same_as<Iterator&>;
    }
{
    // ...
}

// AFTER: Clean concept
template <typename Iterator, typename EndIterator>
    requires CharIterator<Iterator> && Subtractable<EndIterator>
[[nodiscard]] inline constexpr bool has_at_least_bytes(
    Iterator current, EndIterator last, std::size_t n) noexcept
{
    // ...
}

// Or even cleaner:
template <CharIterator Iterator, Subtractable EndIterator>
[[nodiscard]] inline constexpr bool has_at_least_bytes(
    Iterator current, EndIterator last, std::size_t n) noexcept
{
    // ...
}
```

### Defining Concepts

```cpp
/// Concept: Iterator that points to character data
template<typename T>
concept CharIterator = requires(T it) {
    { *it } -> std::convertible_to<char>;
    { ++it } -> std::same_as<T&>;
    requires std::input_or_output_iterator<T>;
};

/// Concept: Can compute distance
template<typename T>
concept Subtractable = requires(T a, T b) {
    { a - b } -> std::convertible_to<std::ptrdiff_t>;
};
```

### Benefits
- ✅ **Much better error messages!**
- ✅ Self-documenting code
- ✅ Reusable constraints
- ✅ Zero runtime cost

---

## 6. inline constexpr - ODR Safety

### What It Does
Makes constants safe to define in headers (One Definition Rule).

### When to Use
✅ **Always** for header-only libraries
✅ Constants in headers
❌ Source files (not needed)

### Examples

```cpp
// ❌ BAD: Multiple definitions if included in multiple TUs
// In header.hpp:
constexpr int SIMD_CAPABILITY_AVX2 = 2;  // ODR violation!

// ✅ GOOD: Safe for headers
inline constexpr int SIMD_CAPABILITY_AVX2 = 2;  // ✅ OK!
```

### Rule of Thumb
- Header file? → Use `inline constexpr`
- Source file? → Use `constexpr` (inline not needed)
- Anywhere? → Use `inline constexpr` (always safe!)

---

## 7. Combining Features

### The Perfect Modern Function

```cpp
/// Detect AVX2 support
/// @return true if CPU supports AVX2
/// @note Result is cached for performance
/// @note C++20: [[nodiscard]] + noexcept for optimization
[[nodiscard]] inline bool has_avx2() noexcept {
    static bool detected = false;
    static bool result = false;

    if (!detected) [[unlikely]] {  // Cold path
        // CPUID check...
        detected = true;
    }

    return result;  // Hot path
}
```

**Features used:**
- `[[nodiscard]]` - Warn if ignored
- `inline` - Header-safe
- `noexcept` - No exceptions (optimization)
- `[[unlikely]]` - Branch hint
- Doxygen comments - Documentation

---

## 8. Migration Guide

### Step 1: Add [[nodiscard]]
```cpp
// BEFORE:
inline bool query() { return true; }

// AFTER:
[[nodiscard]] inline bool query() { return true; }
```

### Step 2: Add noexcept
```cpp
// BEFORE:
[[nodiscard]] inline bool query() { return true; }

// AFTER:
[[nodiscard]] inline bool query() noexcept { return true; }
```

### Step 3: Replace __builtin_expect
```cpp
// BEFORE:
if (__builtin_expect(condition, 1)) { ... }

// AFTER:
if (condition) [[likely]] { ... }
```

### Step 4: Use consteval for compile-time
```cpp
// BEFORE:
constexpr bool is_enabled() { return FLAG; }

// AFTER:
[[nodiscard]] consteval bool is_enabled() noexcept { return FLAG; }
```

### Step 5: Add concepts
```cpp
// BEFORE:
template <typename T>
void process(T value) { ... }

// AFTER:
template <CharIterator T>
void process(T value) { ... }
```

---

## 9. Common Patterns in CTRE

### Pattern 1: SIMD Capability Query
```cpp
[[nodiscard]] inline bool has_xxx() noexcept {
    static bool cached = /* CPUID check */;
    return cached;
}
```

### Pattern 2: Hot/Cold Path
```cpp
if (common_case) [[likely]] {
    // Fast path
} else {
    // Slow path
}

if (!rare_case) [[unlikely]] {
    // Handle rare case
}
```

### Pattern 3: Compile-Time Config
```cpp
[[nodiscard]] consteval bool feature_enabled() noexcept {
    return COMPILE_TIME_FLAG;
}
```

### Pattern 4: Constants in Header
```cpp
inline constexpr int THRESHOLD = 32;
inline constexpr std::size_t SIZE = 64;
```

### Pattern 5: Concept-Constrained Template
```cpp
template <CharIterator Iter, Subtractable Sentinel>
[[nodiscard]] constexpr bool check(Iter it, Sentinel end) noexcept {
    // ...
}
```

---

## 10. Dos and Don'ts

### ✅ DO:
- Use `[[nodiscard]]` on query functions
- Use `[[likely]]` on hot paths (if profiled!)
- Use `noexcept` when truly can't throw
- Use `consteval` for compile-time only
- Use `inline constexpr` for header constants
- Use concepts for template constraints

### ❌ DON'T:
- Use `[[nodiscard]]` on void functions
- Use `[[likely]]` without profiling
- Use `noexcept` if function might throw
- Use `consteval` with runtime data
- Forget `inline` on header constants
- Overcomplicate concepts

---

## 11. Compiler Support

All features require C++20:

| Compiler | Min Version | Flag |
|----------|-------------|------|
| GCC | 10+ | `-std=c++20` |
| Clang | 10+ | `-std=c++20` |
| MSVC | 19.28+ | `/std:c++20` |

CTRE compilation:
```bash
g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 your_code.cpp
```

---

## 12. Resources

### C++20 References:
- [cppreference.com/cpp/20](https://en.cppreference.com/w/cpp/20)
- [Concepts Tutorial](https://en.cppreference.com/w/cpp/language/constraints)
- [Attributes Reference](https://en.cppreference.com/w/cpp/language/attributes)

### CTRE Documentation:
- `MODERNIZATION_COMPLETE.md` - What we modernized
- `MODERNIZATION_PLAN.md` - How we did it
- `concepts.hpp` - All our concepts

---

**Happy Modern C++ Coding!** 🎉

*Remember: Modern C++ isn't just about features—it's about writing fast, safe, maintainable code!*

