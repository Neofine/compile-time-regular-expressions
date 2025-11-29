# Execution Path Investigation

## Question: When is each backend/optimization used?

### Current Code Flow

1. **User calls**: `ctre::match<"pattern">(input)`

2. **Defined in wrapper.hpp**:
   ```cpp
   template <...> constexpr auto match = 
       regular_expression<regex_builder<input>::type, 
                         match_method, ...>()
   ```

3. **match_method::exec() calls**:
   ```cpp
   evaluate(begin, end, Modifier{}, return_type{}, 
            ctll::list<start_mark, RE, assert_subject_end, 
                      end_mark, accept>())
   ```

4. **evaluate() in evaluation.hpp**:
   - Template specialization based on pattern type
   - For `repeat<A, B, set<...>>`:
     * Checks if SIMD applicable (compile-time traits)
     * Checks input size >= threshold (runtime)
     * Calls SIMD functions if eligible
     * Falls back to scalar loop if not

### Where Are These Used?

#### Base CTRE Evaluation (evaluation.hpp)
- **Used by**: ALL benchmarks (ctre::match<>)
- **Contains**: SIMD fast paths for repetitions
- **Handles**: All pattern types

#### BitNFA (bitnfa/)
- **Used by**: NOTHING in current benchmarks
- **Would be used by**: smart_dispatch::match<> (not ctre::match)
- **Status**: Available but not integrated into main API
- **Currently broken**: Depends on deleted literal_fast_path.hpp

#### Smart Dispatch (smart_dispatch.hpp)
- **Used by**: NOTHING in current benchmarks
- **Is**: Wrapper that chooses between ctre::match vs bitnfa::match
- **Status**: Separate API, not integrated

### Investigation Result

**ALL our benchmarks use base CTRE evaluation with SIMD fast paths.**

BitNFA and smart_dispatch are SEPARATE APIs that aren't used.

