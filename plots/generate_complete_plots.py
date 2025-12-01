#!/usr/bin/env python3

import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import numpy as np

# Read the complete benchmark data
df = pd.read_csv('scaling_all_engines_complete.csv')

# Get unique patterns
patterns = df['Pattern'].unique()

print(f"Patterns found: {patterns}")
print(f"Total data points: {len(df)}")
print(f"Engines: {df['Engine'].unique()}")

# Enhanced color scheme for all 6 engines
colors = {
    'CTRE-SIMD': '#2ecc71',     # Green - our optimized version
    'CTRE': '#27ae60',          # Darker green - original
    'Hyperscan': '#e67e22',     # Orange - Intel's library
    'RE2': '#9b59b6',           # Purple - Google's library
    'PCRE2': '#3498db',         # Blue - PCRE2
    'std::regex': '#e74c3c',    # Red - standard library (slowest)
}

markers = {
    'CTRE-SIMD': 'o',
    'CTRE': 's',
    'Hyperscan': '^',
    'RE2': 'D',
    'PCRE2': 'v',
    'std::regex': 'x',
}

# Create a figure for each pattern
for pattern in patterns:
    pattern_data = df[df['Pattern'] == pattern]

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(18, 7))
    fig.suptitle(f'Pattern: `{pattern}` - All Engines Comparison', fontsize=16, fontweight='bold')

    # Plot 1: Time (log-log scale)
    for engine in sorted(pattern_data['Engine'].unique()):
        engine_data = pattern_data[pattern_data['Engine'] == engine]
        ax1.plot(engine_data['Input_Size'], engine_data['Time_ns'],
                marker=markers.get(engine, 'o'), linewidth=2.5, markersize=7,
                label=engine, color=colors.get(engine, '#95a5a6'), alpha=0.9)

    ax1.set_xlabel('Input Size (bytes)', fontsize=13, fontweight='bold')
    ax1.set_ylabel('Time (nanoseconds)', fontsize=13, fontweight='bold')
    ax1.set_title('Processing Time vs Input Size (lower is better)', fontsize=13)
    ax1.set_xscale('log', base=2)
    ax1.set_yscale('log')
    ax1.grid(True, alpha=0.3, which='both', linestyle='--')
    ax1.legend(fontsize=11, loc='upper left', framealpha=0.9)
    ax1.xaxis.set_major_formatter(ticker.FuncFormatter(lambda x, p: f'{int(x)}B'))

    # Plot 2: Throughput (MB/s)
    for engine in sorted(pattern_data['Engine'].unique()):
        engine_data = pattern_data[pattern_data['Engine'] == engine]
        # Calculate throughput: (size_bytes / time_ns) * 1000 = MB/s
        throughput = engine_data['Input_Size'] * 1000 / engine_data['Time_ns']
        ax2.plot(engine_data['Input_Size'], throughput,
                marker=markers.get(engine, 's'), linewidth=2.5, markersize=7,
                label=engine, color=colors.get(engine, '#95a5a6'), alpha=0.9)

    ax2.set_xlabel('Input Size (bytes)', fontsize=13, fontweight='bold')
    ax2.set_ylabel('Throughput (MB/s)', fontsize=13, fontweight='bold')
    ax2.set_title('Throughput vs Input Size (higher is better)', fontsize=13)
    ax2.set_xscale('log', base=2)
    ax2.set_yscale('log')
    ax2.grid(True, alpha=0.3, which='both', linestyle='--')
    ax2.legend(fontsize=11, loc='lower right', framealpha=0.9)
    ax2.xaxis.set_major_formatter(ticker.FuncFormatter(lambda x, p: f'{int(x)}B'))

    # Save figure
    safe_pattern = pattern.replace('[', '').replace(']', '').replace('+', 'plus').replace('*', 'star').replace('-', 'to')
    plt.tight_layout()
    plt.savefig(f'plot_{safe_pattern}.png', dpi=150, bbox_inches='tight')
    print(f'✅ Generated: plot_{safe_pattern}.png')
    plt.close()

# Create summary plot showing CTRE vs CTRE-SIMD speedup across all patterns
fig, ax = plt.subplots(figsize=(16, 9))
fig.suptitle('CTRE-SIMD Speedup vs Original CTRE Across All Patterns', fontsize=16, fontweight='bold')

ctre_simd_data = df[df['Engine'] == 'CTRE-SIMD']
ctre_data = df[df['Engine'] == 'CTRE']

pattern_colors = plt.cm.tab10(np.linspace(0, 1, len(patterns)))

for i, pattern in enumerate(sorted(patterns)):
    simd_pattern_data = ctre_simd_data[ctre_simd_data['Pattern'] == pattern]
    ctre_pattern_data = ctre_data[ctre_data['Pattern'] == pattern]

    speedups = []
    sizes = []

    for size in sorted(simd_pattern_data['Input_Size'].unique()):
        simd_time = simd_pattern_data[simd_pattern_data['Input_Size'] == size]['Time_ns'].values
        ctre_time = ctre_pattern_data[ctre_pattern_data['Input_Size'] == size]['Time_ns'].values

        if len(simd_time) > 0 and len(ctre_time) > 0:
            speedup = ctre_time[0] / simd_time[0]
            speedups.append(speedup)
            sizes.append(size)

    ax.plot(sizes, speedups, marker='o', linewidth=2.5, markersize=6,
            label=pattern, color=pattern_colors[i], alpha=0.8)

# Add 1x reference line
ax.axhline(y=1.0, color='gray', linestyle='--', linewidth=2, alpha=0.5, label='1x (no speedup)')

ax.set_xlabel('Input Size (bytes)', fontsize=13, fontweight='bold')
ax.set_ylabel('Speedup Factor (CTRE / CTRE-SIMD)', fontsize=13, fontweight='bold')
ax.set_title('SIMD Speedup Scaling by Pattern', fontsize=14)
ax.set_xscale('log', base=2)
ax.set_yscale('log')
ax.grid(True, alpha=0.3, which='both', linestyle='--')
ax.legend(fontsize=10, loc='lower right', ncol=2, framealpha=0.9)
ax.xaxis.set_major_formatter(ticker.FuncFormatter(lambda x, p: f'{int(x)}B'))

plt.tight_layout()
plt.savefig('plot_ctre_simd_speedup_all_patterns.png', dpi=150, bbox_inches='tight')
print(f'✅ Generated: plot_ctre_simd_speedup_all_patterns.png')
plt.close()

# Create comprehensive comparison grid
fig, axes = plt.subplots(2, 3, figsize=(20, 12))
fig.suptitle('All Engines Performance Comparison', fontsize=16, fontweight='bold')

axes = axes.flatten()

for idx, pattern in enumerate(sorted(patterns)):
    ax = axes[idx]
    pattern_data = df[df['Pattern'] == pattern]

    for engine in sorted(pattern_data['Engine'].unique()):
        engine_data = pattern_data[pattern_data['Engine'] == engine]
        throughput = engine_data['Input_Size'] * 1000 / engine_data['Time_ns']
        ax.plot(engine_data['Input_Size'], throughput,
                marker=markers.get(engine, 'o'), linewidth=2, markersize=5,
                label=engine, color=colors.get(engine, '#95a5a6'), alpha=0.85)

    ax.set_xlabel('Input Size (bytes)', fontsize=10)
    ax.set_ylabel('Throughput (MB/s)', fontsize=10)
    ax.set_title(f'`{pattern}`', fontsize=12, fontweight='bold')
    ax.set_xscale('log', base=2)
    ax.set_yscale('log')
    ax.grid(True, alpha=0.3, which='both')
    ax.legend(fontsize=8, loc='lower right')
    ax.xaxis.set_major_formatter(ticker.FuncFormatter(lambda x, p: f'{int(x)}B'))

plt.tight_layout()
plt.savefig('plot_all_engines_grid.png', dpi=150, bbox_inches='tight')
print(f'✅ Generated: plot_all_engines_grid.png')
plt.close()

print(f'\n✅ All plots generated successfully!')
print(f'Total graphs: {len(patterns) + 2}')
