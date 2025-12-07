# Code Review Issues and Fixes

## Issues Found

### 1. **set -e is too strict** ⚠️
**Problem**: Script exits on any error, even minor ones like one CPU governor failing
**Impact**: Script fails completely if any CPU can't set governor
**Fix**: Use `set +e` for non-critical operations, check results

### 2. **IRQ CPU parsing doesn't handle ranges** ⚠️
**Problem**: `isolcpus=2-4` format not parsed correctly
**Impact**: IRQ affinity won't work correctly with CPU ranges
**Fix**: Add range expansion logic

### 3. **chrt requires root/capabilities** ⚠️
**Problem**: `chrt -f 50` requires root or CAP_SYS_NICE, script doesn't check
**Impact**: Real-time priority fails silently, falls back to nice
**Fix**: Check if chrt will work, warn if not

### 4. **NUMA node hardcoded** ⚠️
**Problem**: Always uses NUMA_NODE=0, but isolated CPUs might be on different node
**Impact**: Cross-NUMA memory access, slower performance
**Fix**: Detect which NUMA node isolated CPUs are on

### 5. **All IRQs on one CPU** ⚠️
**Problem**: All IRQs moved to first non-isolated CPU, could overload it
**Impact**: IRQ handling CPU becomes bottleneck
**Fix**: Distribute IRQs across non-isolated CPUs

### 6. **ASLR not restored** ⚠️
**Problem**: ASLR disabled but original value not saved/restored
**Impact**: Security risk if script crashes, ASLR stays disabled
**Fix**: Save original value, provide restore script

### 7. **chrt priority 50 might be too high** ⚠️
**Problem**: Real-time priority 50 is very high, could starve system
**Impact**: System might become unresponsive
**Fix**: Use lower priority (1-10) or make configurable

### 8. **No IRQ move verification** ⚠️
**Problem**: IRQ moves attempted but not verified
**Impact**: Don't know if IRQ affinity actually worked
**Fix**: Verify IRQs were moved after attempting

### 9. **Error handling in eval** ⚠️
**Problem**: `eval "$CMD"` might fail silently
**Impact**: Benchmarks might not run but script continues
**Fix**: Better error checking

### 10. **CPU range parsing in isolcpus** ⚠️
**Problem**: Kernel supports `isolcpus=2-4,6` but our parsing doesn't
**Impact**: Wrong CPUs identified as isolated

## Mutually Exclusive/Conflicting Settings

### ✅ No conflicts found:
- `numactl --physcpubind` vs `taskset`: We use EITHER, not both
- `chrt` vs `nice`: We use EITHER, not both  
- `--membind` and `--physcpubind`: Both can be used together (correct)

### ⚠️ Potential issues:
- Real-time priority + isolated CPUs: Should work, but high priority might interfere with isolation
- IRQ affinity + isolated CPUs: Correct approach, but need to verify it works

