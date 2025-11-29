#!/usr/bin/env python3
import subprocess
import re

# Get current benchmark results
result = subprocess.run(['./run_individual_benchmarks.sh'], 
                       capture_output=True, text=True, timeout=300)

patterns = {}
for line in result.stdout.split('\n'):
    match = re.search(r'\[(\d+)/\d+\]\s+([^:]+):\s+([\d.]+)x', line)
    if match:
        name = match.group(2).strip()
        speedup = float(match.group(3))
        patterns[name] = speedup

# Read pattern definitions from benchmark files
import os
import glob

pattern_defs = {}
for bench_file in glob.glob('tests/individual_benchmarks/*_bench.cpp'):
    with open(bench_file, 'r') as f:
        content = f.read()
        # Extract pattern name and regex
        name_match = re.search(r'cout.*"([^"]+)@@([^"]+)@@', content)
        if name_match:
            name = name_match.group(1)
            pattern = name_match.group(2)
            pattern_defs[name] = pattern

print("╔═══════════════════════════════════════════════════════════════════════╗")
print("║        Finding Optimization Targets for Teddy/Switch               ║")
print("╚═══════════════════════════════════════════════════════════════════════╝")
print()

# Categorize opportunities
alternations = []
literals = []
short_patterns = []
underperformers = []

for name, pattern in pattern_defs.items():
    speedup = patterns.get(name, 0)
    
    # Alternations (|)
    if '|' in pattern:
        alternations.append((name, pattern, speedup))
    
    # Pure literals (no special chars)
    elif not any(c in pattern for c in '[]()+*?{}.^$\\'):
        literals.append((name, pattern, speedup))
    
    # Short patterns that might need special handling
    if '_16' in name or '_32' in name:
        if speedup < 5.0:
            short_patterns.append((name, pattern, speedup))
    
    # Underperformers
    if speedup < 2.0 and speedup > 0:
        underperformers.append((name, pattern, speedup))

print("🎯 TARGET 1: ALTERNATION PATTERNS (can use switch-opt)")
print("=" * 70)
print(f"Found {len(alternations)} patterns with alternations (|)")
print()
for name, pattern, speedup in sorted(alternations, key=lambda x: x[2]):
    status = "✅ Optimized" if speedup > 1.4 else "❌ OPTIMIZE!"
    print(f"  {name:25s} {speedup:5.2f}x  {status}")
    print(f"    Pattern: {pattern[:60]}")
print()

print("🎯 TARGET 2: PURE LITERAL PATTERNS (can use direct comparison)")
print("=" * 70)
print(f"Found {len(literals)} pure literal patterns")
print()
for name, pattern, speedup in sorted(literals, key=lambda x: x[2])[:10]:
    status = "✅ OK" if speedup > 1.5 else "❌ OPTIMIZE!"
    print(f"  {name:25s} {speedup:5.2f}x  {status}")
    print(f"    Pattern: {pattern[:60]}")
print()

print("🎯 TARGET 3: SHORT PATTERN UNDERPERFORMERS (< 5x for 16/32 bytes)")
print("=" * 70)
print(f"Found {len(short_patterns)} short patterns with low speedup")
print()
for name, pattern, speedup in sorted(short_patterns, key=lambda x: x[2])[:10]:
    print(f"  {name:25s} {speedup:5.2f}x  ❌ OPTIMIZE!")
    print(f"    Pattern: {pattern[:60]}")
print()

print("🎯 TARGET 4: ALL UNDERPERFORMERS (< 2.0x)")
print("=" * 70)
print(f"Found {len(underperformers)} patterns below 2.0x")
print()
for name, pattern, speedup in sorted(underperformers, key=lambda x: x[2]):
    print(f"  {name:25s} {speedup:5.2f}x")
    print(f"    Pattern: {pattern[:60]}")
print()

print("═══════════════════════════════════════════════════════════════════════")
print(" OPTIMIZATION STRATEGY")
print("═══════════════════════════════════════════════════════════════════════")
print()

# Calculate potential impact
alt_count = len([a for a in alternations if a[2] < 1.4])
lit_count = len([l for l in literals if l[2] < 1.5])
short_count = len(short_patterns)
under_count = len(underperformers)

print(f"1. Apply switch-opt to {alt_count} more alternations")
print(f"   Expected gain: +{alt_count * 0.2:.2f}x (if each gets +0.2x)")
print()
print(f"2. Optimize {lit_count} literal patterns with direct comparison")
print(f"   Expected gain: +{lit_count * 0.3:.2f}x (if each gets +0.3x)")
print()
print(f"3. Fix {short_count} short pattern issues")
print(f"   Expected gain: +{short_count * 0.3:.2f}x (if each gets +0.3x)")
print()
print(f"4. Optimize {under_count} underperformers")
print(f"   Expected gain: +{under_count * 0.5:.2f}x (if each gets +0.5x)")
print()

total_potential = (alt_count * 0.2 + lit_count * 0.3 + short_count * 0.3 + under_count * 0.5) / len(patterns)
print(f"TOTAL POTENTIAL GAIN: +{total_potential:.2f}x")
print(f"Current: 10.08x → Target: {10.08 + total_potential:.2f}x")
print()

