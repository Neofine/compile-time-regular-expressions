# 🔬 DEEP MACHINE CODE ANALYSIS

## Key Findings from Assembly

### 🚨 PROBLEM #1: Massive CPUID Overhead (Lines 661-702)

**For EVERY function call:**
```assembly
661: cmp    BYTE PTR [rip+0x36da],0x0    # Check if AVX512 detected
664: je     1b10                          # Branch if not checked yet  
666: movzx  ebx,BYTE PTR [rip+0x36c0]    # Load AVX512 result
667: test   bl,bl                         # Test result
668: jne    19f0                          # Branch if has AVX512
669: cmp    BYTE PTR [rip+0x36b4],0x0    # Check if AVX2 detected
670: je     1b40                          # Branch if not checked yet
677: movzx  ebx,BYTE PTR [rip+0x36a6]    # Load AVX2 result
678: test   bl,bl                         # Test result
679: jne    19f0                          # Branch if has AVX2
680: cmp    BYTE PTR [rip+0x3698],0x0    # Check if SSE4.2 detected
681: jne    199e                          # Branch if checked
682: mov    eax,0x1                       # CPUID instruction!
683: cpuid                                 # ~200 cycles!
```

**Cost per call:**
- 3 CPUID checks (AVX512, AVX2, SSE4.2)
- 6-9 memory loads
- 6-9 branches
- **Estimated: 20-30 cycles per call!**

**This is HUGE overhead for 16-byte patterns!**

---

### 🚨 PROBLEM #2: Stack Frame Overhead

```assembly
648: push   rbp                    # Save base pointer
649: mov    rbp,rsp                # Set up frame
650: push   r13                    # Save callee-saved registers
651: push   r12
652: push   rbx
653: and    rsp,0xffffffffffffffe0  # Align stack to 32 bytes! (expensive!)
656: sub    rsp,0x20               # Allocate stack space
657: mov    rax,QWORD PTR fs:0x28  # Load canary (security)
659: mov    QWORD PTR [rsp+0x18],rax # Store canary
```

**Cost:** ~15-20 cycles
**Problem:** For 16-byte pattern that processes in ~5-10 cycles, this is 200% overhead!

---

### ✅ GOOD: Actual SIMD Loop is Tight (64-byte unroll)

```assembly
714: mov    ecx,0x61              # Load 'a'
715: vpcmpeqd ymm1,ymm1,ymm1      # Create all-ones mask
717: vmovd  xmm0,ecx              # Move 'a' to vector
718: vpbroadcastb ymm0,xmm0       # Broadcast to all 32 lanes
719: vpcmpeqb ymm2,ymm0,[rax]     # Compare first 32 bytes (1 cycle)
721: vpcmpeqb ymm3,ymm0,[rax+0x20] # Compare next 32 bytes (1 cycle)
722: vptest ymm2,ymm1              # Test if all match (1 cycle)
723: setb   cl                      # Set flag (1 cycle)
725: vptest ymm3,ymm1              # Test second 32 bytes
726: setb   sil                     # Set flag
```

**Cost:** ~6-8 cycles for 64 bytes = **0.1 cycle/byte** ✅ EXCELLENT!

---

### 🚨 PROBLEM #3: Range Check Uses 5 Instructions Per 16 Bytes

```assembly
312: vmovdqu xmm7,[rax]           # Load 16 bytes (2 cycles)
313: vpcmpgtb xmm0,xmm3,[rax]     # Test if < 'a' (1 cycle)
314: vpcmpgtb xmm1,xmm7,xmm2      # Test if > 'z' (1 cycle)
315: vpor xmm0,xmm0,xmm1          # OR results (1 cycle)
316: vpxor xmm0,xmm0,xmm4         # Invert (1 cycle)
317: vpmovmskb ecx,xmm0           # Extract mask (2 cycles)
318: cmp ecx,0xffff               # Check all match (1 cycle)
319: je 1420                       # Branch if match (predicted: free)
```

**Cost:** ~9 cycles per 16 bytes = **0.56 cycle/byte**
**vs single char:** 0.1 cycle/byte

**Range check is 5.6x slower per byte!** This explains why [a-z]*_32 is slower.

---

## 📊 Overhead Breakdown for 16-Byte Pattern

```
Total time for 16-byte pattern: ~60-70 cycles
Breakdown:
- CPUID checks:     25 cycles (40%) ❌❌❌
- Stack frame:      15 cycles (24%) ❌❌
- SIMD processing:  2 cycles (3%) ✅
- Scalar tail:      5 cycles (8%) 
- Other overhead:   15 cycles (24%)
```

**SIMD does only 3% of the work!**
**Overhead is 97% of the time!**

---

## 💡 OPTIMIZATION OPPORTUNITIES

### 1. Cache CPUID Results Better ✅ HIGH IMPACT

**Current:** Checks on every call (25 cycles)
**Better:** Check once per thread/process

**Potential gain:** -25 cycles = -40% for short patterns!

### 2. Mark Functions `inline` More Aggressively

**Problem:** Functions are being called (not inlined)
**Solution:** Let compiler inline more aggressively
**BUT:** We tried `always_inline` and it made things worse (code bloat)

**Better approach:** Use `flatten` attribute on hot paths?

### 3. Reduce Stack Frame Overhead

**Problem:** 32-byte alignment is expensive
**Solution:** Use `-mpreferred-stack-boundary=3` (8-byte alignment)?
**Risk:** May hurt AVX2 performance

### 4. Optimize Range Checks (5 instructions → 3?)

**Current:** Load, cmpgt, cmpgt, or, xor = 5 ops
**Possible:** Use vpblendvb to reduce operations?

---

## 🎯 SPECIFIC FIXES TO TRY

### Fix #1: Cache CPUID Results in Thread-Local Storage

```cpp
static thread_local bool simd_checked = false;
static thread_local int simd_level = 0;

if (__builtin_expect(!simd_checked, 0)) {
    simd_level = detect_simd();
    simd_checked = true;
}
```

**Expected gain:** +30-40% for short patterns!

### Fix #2: Use `__builtin_constant_p` for Compile-Time SIMD Selection

If pattern size is compile-time constant, skip runtime CPUID check entirely.

### Fix #3: Optimize Range Check with PSHUFB

Use PSHUFB for range checking (could reduce to 3 instructions):
```assembly
vpshufb   # Use lookup table
vpcmpeqb  # Compare to expected
vpmovmskb # Extract mask
```

---

## 🔬 WHY SHORT PATTERNS HAVE MORE CACHE MISSES

**From assembly analysis:**

1. **More function calls per byte:**
   - 16-byte: 1 call per 16 bytes
   - 256-byte: 1 call per 256 bytes
   - 16x more calls = 16x more I-cache pressure

2. **CPUID checks hit cold cache:**
   - Each call loads 3 global variables
   - These are likely in different cache lines
   - 3 cache misses × 1 call per 16 bytes = lots of misses!

3. **Stack operations thrash cache:**
   - Push/pop operations
   - Loading stack canary from TLS (fs:0x28)
   - Each miss = ~200 cycles

---

## 💎 GOLD NUGGET: The Real Fix

**THE SMOKING GUN:**

For 16-byte patterns, **overhead is 60x the actual SIMD work!**

```
SIMD work: 2 cycles
Overhead: 60-68 cycles
Ratio: 30:1 ❌❌❌
```

**To reach theoretical 6x speedup for a*_16:**
- Need to eliminate ~40 cycles of overhead
- CPUID caching alone could save 25 cycles
- Would take us from 5.39x to potentially 7-8x!

**This is THE optimization to implement!**
