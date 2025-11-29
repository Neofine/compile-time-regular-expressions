#!/usr/bin/env python3
import re
import subprocess

# Run benchmark and capture patterns
result = subprocess.run(['./run_individual_benchmarks.sh'], 
                       capture_output=True, text=True, timeout=300)

patterns = {}
for line in result.stdout.split('\n'):
    if '@@' in line:
        parts = line.split('@@')
        if len(parts) >= 4:
            name = parts[0].strip()
            pattern = parts[1].strip()
            try:
                speedup = float(parts[2].strip().replace('x', ''))
                patterns[name] = {'pattern': pattern, 'speedup': speedup}
            except:
                pass

# Categorize patterns
categories = {
    'literal_alternation': [],
    'complex_alternation': [],
    'character_class': [],
    'repetition': [],
    'sequence': [],
}

for name, data in patterns.items():
    pattern = data['pattern']
    speedup = data['speedup']
    
    # Check if it's a simple literal alternation (Tom|Sawyer|...)
    if '|' in pattern and not any(c in pattern for c in '[]()+*?{}.^$\\'):
        categories['literal_alternation'].append((name, pattern, speedup))
    # Complex alternation (has | and other regex features)
    elif '|' in pattern:
        categories['complex_alternation'].append((name, pattern, speedup))
    # Character class repetition
    elif '[' in pattern and ('+' in pattern or '*' in pattern or '{' in pattern):
        categories['character_class'].append((name, pattern, speedup))
    # Simple repetition (a*, a+, a{32})
    elif any(c in pattern for c in '+*{'):
        categories['repetition'].append((name, pattern, speedup))
    else:
        categories['sequence'].append((name, pattern, speedup))

print("╔═══════════════════════════════════════════════════════════════════════╗")
print("║              Pattern Analysis - Where Can Teddy Help?                ║")
print("╚═══════════════════════════════════════════════════════════════════════╝")
print()

print("LITERAL ALTERNATIONS (Pure literals like Tom|Sawyer|...)")
print("=" * 70)
print(f"Count: {len(categories['literal_alternation'])}")
if categories['literal_alternation']:
    for name, pattern, speedup in sorted(categories['literal_alternation'], key=lambda x: x[2]):
        print(f"  {name:25s} {speedup:5.2f}x  {pattern[:40]}")
    avg = sum(s for _, _, s in categories['literal_alternation']) / len(categories['literal_alternation'])
    print(f"\n  Average speedup: {avg:.2f}x")
    print(f"  🎯 TARGET: These should benefit from switch-opt!")
else:
    print("  None found.")
print()

print("COMPLEX ALTERNATIONS (Alternations with regex features)")
print("=" * 70)
print(f"Count: {len(categories['complex_alternation'])}")
if categories['complex_alternation']:
    for name, pattern, speedup in sorted(categories['complex_alternation'], key=lambda x: x[2]):
        print(f"  {name:25s} {speedup:5.2f}x  {pattern[:40]}")
    avg = sum(s for _, _, s in categories['complex_alternation']) / len(categories['complex_alternation'])
    print(f"\n  Average speedup: {avg:.2f}x")
    print(f"  🎯 TARGET: These might benefit from BitNFA or hybrid!")
else:
    print("  None found.")
print()

# Find underperformers (< 2.0x)
print("UNDERPERFORMING PATTERNS (< 2.0x speedup)")
print("=" * 70)
underperformers = [(n, d['pattern'], d['speedup']) for n, d in patterns.items() if d['speedup'] < 2.0]
underperformers.sort(key=lambda x: x[2])
for name, pattern, speedup in underperformers[:15]:
    print(f"  {name:25s} {speedup:5.2f}x  {pattern[:40]}")
print()

print("OPPORTUNITY ANALYSIS")
print("=" * 70)
lit_alt_count = len(categories['literal_alternation'])
complex_alt_count = len(categories['complex_alternation'])
total_alt = lit_alt_count + complex_alt_count
underperformer_count = len([p for p in patterns.values() if p['speedup'] < 2.0])

print(f"Literal alternations:      {lit_alt_count:3d} patterns 🎯 Switch-opt")
print(f"Complex alternations:      {complex_alt_count:3d} patterns 🎯 BitNFA/Teddy")
print(f"Underperformers (< 2.0x):  {underperformer_count:3d} patterns 🎯 Investigate")
print()
print(f"If we optimize all alternations → potential {total_alt}+ pattern improvements!")
print()

