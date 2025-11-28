#!/usr/bin/env python3

# The issue: patterns with | break the parsing
# Fix: Use a proper CSV format or different delimiter

with open('results/individual/all_results.txt', 'r') as f:
    lines = f.readlines()

print("name\tpattern\tsimd_ns\tdesc\tnosimd_ns\tspeedup")
for line in lines:
    parts = line.strip().split('|')
    if len(parts) >= 5:
        name = parts[0]
        # Pattern might contain | for alternations
        # Try to detect if this happened
        if len(parts) == 6:
            # Normal case
            pattern, simd, desc, nosimd, speedup = parts[1:]
        elif len(parts) > 6:
            # Pattern contains |, need to rejoin
            pattern = '|'.join(parts[1:-4])
            simd, desc, nosimd, speedup = parts[-4:]
        else:
            continue
        
        speedup_num = float(speedup.replace('x', ''))
        print(f"{name}\t{pattern}\t{simd}\t{desc}\t{nosimd}\t{speedup_num:.2f}x")
