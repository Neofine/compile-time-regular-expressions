#!/usr/bin/env python3
"""
Find patterns in master_benchmark.cpp that would benefit from Teddy algorithm.

Teddy is best for: literal alternations like "foo|bar|baz"
"""

import re
import sys

def analyze_for_teddy(pattern):
    """Determine if a pattern would benefit from Teddy."""
    
    # Teddy benefits:
    # 1. Pure literal alternations: "foo|bar|baz"
    # 2. Prefix literals in alternations: "foo[a-z]+|bar[0-9]+"
    
    # Check for alternations
    if '|' not in pattern:
        return None, "No alternation"
    
    # Split by | to get branches
    branches = pattern.split('|')
    
    # Analyze each branch
    pure_literals = []
    has_prefix_literals = []
    
    for branch in branches:
        # Remove capture groups for analysis
        branch_clean = re.sub(r'[()]', '', branch)
        
        # Check if it's a pure literal (just letters/chars, no special regex)
        if re.match(r'^[a-zA-Z]+$', branch_clean):
            pure_literals.append(branch_clean)
        # Check if it has a literal prefix
        elif match := re.match(r'^([a-zA-Z]{2,})', branch_clean):
            has_prefix_literals.append(match.group(1))
    
    # Determine benefit level
    if len(pure_literals) == len(branches):
        return "HIGH", f"Pure literal alternation: {len(pure_literals)} literals"
    elif len(pure_literals) + len(has_prefix_literals) >= 2:
        return "MEDIUM", f"Has literal components: {len(pure_literals)} pure + {len(has_prefix_literals)} prefix"
    else:
        return "LOW", "Alternation has few/no literals"

def main():
    print("╔═══════════════════════════════════════════════════════════════════════╗")
    print("║    Finding Teddy Algorithm Candidates                                ║")
    print("╚═══════════════════════════════════════════════════════════════════════╝")
    print()
    print("Teddy is SIMD algorithm for multi-substring search (literal alternations).")
    print()
    
    # Parse master_benchmark.cpp
    patterns = {}
    
    try:
        with open('tests/master_benchmark.cpp', 'r') as f:
            content = f.read()
            
            # Find all BENCH() calls
            bench_pattern = r'BENCH\("([^"]+)",\s*"([^"]+)"'
            for match in re.finditer(bench_pattern, content):
                name = match.group(1)
                pattern = match.group(2)
                patterns[name] = pattern
    except FileNotFoundError:
        print("Error: Could not find tests/master_benchmark.cpp")
        return 1
    
    # Analyze each pattern
    teddy_candidates = {
        "HIGH": [],
        "MEDIUM": [],
        "LOW": []
    }
    
    non_alternations = []
    
    for name, pattern in sorted(patterns.items()):
        benefit, reason = analyze_for_teddy(pattern)
        
        if benefit:
            teddy_candidates[benefit].append((name, pattern, reason))
        else:
            non_alternations.append((name, pattern))
    
    # Display results
    print("═══════════════════════════════════════════════════════════════════════")
    print(" HIGH BENEFIT (Pure Literal Alternations)")
    print("═══════════════════════════════════════════════════════════════════════")
    print()
    
    if teddy_candidates["HIGH"]:
        for name, pattern, reason in teddy_candidates["HIGH"]:
            print(f"✅ {name}")
            print(f"   Pattern: {pattern}")
            print(f"   Benefit: {reason}")
            print(f"   Expected: 3-10x faster with Teddy! 🔥")
            print()
    else:
        print("(No patterns found)")
        print()
    
    print("═══════════════════════════════════════════════════════════════════════")
    print(" MEDIUM BENEFIT (Partial Literal Alternations)")
    print("═══════════════════════════════════════════════════════════════════════")
    print()
    
    if teddy_candidates["MEDIUM"]:
        for name, pattern, reason in teddy_candidates["MEDIUM"]:
            print(f"⚠️  {name}")
            print(f"   Pattern: {pattern}")
            print(f"   Benefit: {reason}")
            print(f"   Expected: 1.5-3x faster with Teddy prefix scan")
            print()
    else:
        print("(No patterns found)")
        print()
    
    print("═══════════════════════════════════════════════════════════════════════")
    print(" LOW BENEFIT (Complex Alternations)")
    print("═══════════════════════════════════════════════════════════════════════")
    print()
    
    if teddy_candidates["LOW"]:
        for name, pattern, reason in teddy_candidates["LOW"]:
            print(f"❌ {name}")
            print(f"   Pattern: {pattern}")
            print(f"   Benefit: {reason}")
            print()
    else:
        print("(No patterns found)")
        print()
    
    # Summary
    total_alternations = (len(teddy_candidates["HIGH"]) + 
                         len(teddy_candidates["MEDIUM"]) + 
                         len(teddy_candidates["LOW"]))
    
    print("═══════════════════════════════════════════════════════════════════════")
    print(" SUMMARY")
    print("═══════════════════════════════════════════════════════════════════════")
    print()
    print(f"Total patterns: {len(patterns)}")
    print(f"Alternations: {total_alternations}")
    print(f"  • HIGH benefit: {len(teddy_candidates['HIGH'])} patterns")
    print(f"  • MEDIUM benefit: {len(teddy_candidates['MEDIUM'])} patterns")
    print(f"  • LOW benefit: {len(teddy_candidates['LOW'])} patterns")
    print(f"Non-alternations: {len(non_alternations)}")
    print()
    
    high_count = len(teddy_candidates['HIGH'])
    medium_count = len(teddy_candidates['MEDIUM'])
    
    if high_count > 0:
        print("🔥 RECOMMENDATION: Teddy would significantly benefit:")
        for name, _, _ in teddy_candidates["HIGH"]:
            print(f"   • {name} (pure literals)")
        print()
        print(f"   Expected impact: {high_count} patterns → 3-10x faster!")
        print(f"   Average improvement: +{high_count * 5 / len(patterns):.2f}x overall")
        print()
    
    if medium_count > 0:
        print("⚠️  Partial benefit for:")
        for name, _, _ in teddy_candidates["MEDIUM"]:
            print(f"   • {name} (prefix literals)")
        print()
    
    if high_count == 0 and medium_count == 0:
        print("❌ LIMITED BENEFIT: Few patterns would benefit from Teddy.")
        print("   Consider optimizing BitNFA instead for alternations.")
        print()
    
    print("═══════════════════════════════════════════════════════════════════════")
    
    return 0

if __name__ == '__main__':
    sys.exit(main())

