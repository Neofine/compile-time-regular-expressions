#!/usr/bin/env python3

import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import numpy as np

# Read the benchmark data
df = pd.read_csv('scaling_all_engines.csv')

# Get unique patterns
patterns = df['Pattern'].unique()

# Color scheme
colors = {
    'CTRE-SIMD': '#2ecc71',  # Green - our optimized version
    'std::regex': '#e74c3c',  # Red - standard library
    'PCRE2': '#3498db',       # Blue - PCRE2
}

# Create a figure for each pattern
for pattern in patterns:
    pattern_data = df[df['Pattern'] == pattern]

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 6))
    fig.suptitle(f'Pattern: {pattern}', fontsize=16, fontweight='bold')

    # Plot 1: Time (linear scale)
    for engine in pattern_data['Engine'].unique():
        engine_data = pattern_data[pattern_data['Engine'] == engine]
        ax1.plot(engine_data['Input_Size'], engine_data['Time_ns'],
                marker='o', linewidth=2, markersize=6,
                label=engine, color=colors.get(engine, '#95a5a6'))

    ax1.set_xlabel('Input Size (bytes)', fontsize=12, fontweight='bold')
    ax1.set_ylabel('Time (nanoseconds)', fontsize=12, fontweight='bold')
    ax1.set_title('Processing Time vs Input Size', fontsize=14)
    ax1.set_xscale('log', base=2)
    ax1.set_yscale('log')
    ax1.grid(True, alpha=0.3, which='both')
    ax1.legend(fontsize=10, loc='upper left')

    # Format x-axis to show actual sizes
    ax1.xaxis.set_major_formatter(ticker.FuncFormatter(lambda x, p: f'{int(x)}'))

    # Plot 2: Throughput (MB/s)
    for engine in pattern_data['Engine'].unique():
        engine_data = pattern_data[pattern_data['Engine'] == engine]
        # Calculate throughput: (size_bytes / time_ns) * 1000 = MB/s
        throughput = engine_data['Input_Size'] * 1000 / engine_data['Time_ns']
        ax2.plot(engine_data['Input_Size'], throughput,
                marker='s', linewidth=2, markersize=6,
                label=engine, color=colors.get(engine, '#95a5a6'))

    ax2.set_xlabel('Input Size (bytes)', fontsize=12, fontweight='bold')
    ax2.set_ylabel('Throughput (MB/s)', fontsize=12, fontweight='bold')
    ax2.set_title('Throughput vs Input Size', fontsize=14)
    ax2.set_xscale('log', base=2)
    ax2.set_yscale('log')
    ax2.grid(True, alpha=0.3, which='both')
    ax2.legend(fontsize=10, loc='lower right')

    # Format x-axis
    ax2.xaxis.set_major_formatter(ticker.FuncFormatter(lambda x, p: f'{int(x)}'))

    # Save figure
    safe_pattern = pattern.replace('[', '').replace(']', '').replace('+', 'plus').replace('*', 'star').replace('-', 'to')
    plt.tight_layout()
    plt.savefig(f'scaling_plot_{safe_pattern}.png', dpi=150, bbox_inches='tight')
    print(f'✅ Generated: scaling_plot_{safe_pattern}.png')
    plt.close()

# Create summary plot showing all patterns for CTRE-SIMD
fig, ax = plt.subplots(figsize=(14, 8))
fig.suptitle('CTRE-SIMD: Scaling Performance Across All Patterns', fontsize=16, fontweight='bold')

ctre_simd_data = df[df['Engine'] == 'CTRE-SIMD']
pattern_colors = plt.cm.tab10(np.linspace(0, 1, len(patterns)))

for i, pattern in enumerate(patterns):
    pattern_data = ctre_simd_data[ctre_simd_data['Pattern'] == pattern]
    throughput = pattern_data['Input_Size'] * 1000 / pattern_data['Time_ns']
    ax.plot(pattern_data['Input_Size'], throughput,
            marker='o', linewidth=2, markersize=5,
            label=pattern, color=pattern_colors[i])

ax.set_xlabel('Input Size (bytes)', fontsize=12, fontweight='bold')
ax.set_ylabel('Throughput (MB/s)', fontsize=12, fontweight='bold')
ax.set_title('All Patterns Performance', fontsize=14)
ax.set_xscale('log', base=2)
ax.set_yscale('log')
ax.grid(True, alpha=0.3, which='both')
ax.legend(fontsize=9, loc='lower right', ncol=2)
ax.xaxis.set_major_formatter(ticker.FuncFormatter(lambda x, p: f'{int(x)}'))

plt.tight_layout()
plt.savefig('scaling_plot_all_patterns_ctre_simd.png', dpi=150, bbox_inches='tight')
print(f'✅ Generated: scaling_plot_all_patterns_ctre_simd.png')
plt.close()

# Create speedup comparison plot
fig, axes = plt.subplots(2, 3, figsize=(18, 12))
fig.suptitle('CTRE-SIMD Speedup vs Competitors', fontsize=16, fontweight='bold')

axes = axes.flatten()

for idx, pattern in enumerate(patterns):
    ax = axes[idx]
    pattern_data = df[df['Pattern'] == pattern]

    # Get CTRE-SIMD times as baseline
    ctre_simd = pattern_data[pattern_data['Engine'] == 'CTRE-SIMD']

    for engine in ['std::regex', 'PCRE2']:
        engine_data = pattern_data[pattern_data['Engine'] == engine]

        # Calculate speedup
        speedup = []
        sizes = []
        for size in ctre_simd['Input_Size'].values:
            ctre_time = ctre_simd[ctre_simd['Input_Size'] == size]['Time_ns'].values[0]
            engine_time = engine_data[engine_data['Input_Size'] == size]['Time_ns'].values[0]
            speedup.append(engine_time / ctre_time)
            sizes.append(size)

        ax.plot(sizes, speedup, marker='o', linewidth=2, markersize=5,
                label=f'vs {engine}', color=colors.get(engine, '#95a5a6'))

    # Add 1x line
    ax.axhline(y=1.0, color='gray', linestyle='--', linewidth=1, alpha=0.5, label='1x (break-even)')

    ax.set_xlabel('Input Size (bytes)', fontsize=10)
    ax.set_ylabel('Speedup (x)', fontsize=10)
    ax.set_title(f'Pattern: {pattern}', fontsize=11, fontweight='bold')
    ax.set_xscale('log', base=2)
    ax.set_yscale('log')
    ax.grid(True, alpha=0.3, which='both')
    ax.legend(fontsize=8)
    ax.xaxis.set_major_formatter(ticker.FuncFormatter(lambda x, p: f'{int(x)}'))

plt.tight_layout()
plt.savefig('scaling_plot_speedups.png', dpi=150, bbox_inches='tight')
print(f'✅ Generated: scaling_plot_speedups.png')
plt.close()

print(f'\n✅ All plots generated successfully!')
print(f'Total graphs: {len(patterns) + 2}')
