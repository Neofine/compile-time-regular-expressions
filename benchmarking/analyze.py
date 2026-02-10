#!/usr/bin/env python3
"""Analyze benchmark CSV results and generate publication-quality plots."""

import argparse
import sys
from pathlib import Path

try:
    import pandas as pd
    import matplotlib.pyplot as plt
    import matplotlib as mpl
    import numpy as np
    import seaborn as sns
except ImportError:
    print("Install: pip install pandas matplotlib numpy seaborn")
    sys.exit(1)

# Consistent engine ordering and colors
ENGINES = ['CTRE-SIMD', 'CTRE-Scalar', 'CTRE', 'RE2', 'PCRE2', 'Hyperscan', 'std::regex']
ENGINE_COLORS = {
    'CTRE-SIMD': '#2166ac',
    'CTRE-Scalar': '#67a9cf',
    'CTRE': '#d1e5f0',
    'RE2': '#d6604d',
    'PCRE2': '#8073ac',
    'Hyperscan': '#f4a582',
    'std::regex': '#666666',
}

# Plot parameters (matching tmp2.py style)
PLOT_PARAMS = {
    'bar_width': 0.15,
    'bar_edge_color': 'white',
    'bar_edge_width': 0.5,
    'heatmap_linewidth': 2,
    'heatmap_linecolor': 'white',
    'save_dpi': 300,
}

def setup_style():
    """Configure matplotlib for publication-quality figures."""
    plt.style.use('seaborn-v0_8-whitegrid')
    mpl.rcParams.update({
        'font.family': 'sans-serif',
        'font.size': 10,
        'axes.titlesize': 12,
        'axes.titleweight': 'bold',
        'axes.labelsize': 10,
        'xtick.labelsize': 9,
        'ytick.labelsize': 9,
        'legend.fontsize': 9,
        'figure.dpi': 150,
        'savefig.dpi': 150,
        'savefig.bbox': 'tight',
        'savefig.facecolor': 'white',
    })

def get_heatmap_cmap():
    """Diverging colormap: dark red (slow) -> yellow (1x) -> dark green (fast)."""
    from matplotlib.colors import LinearSegmentedColormap
    colors = [
        '#8b0000',  # 0.0 - dark red (very slow)
        '#d73027',  # 0.2 - red
        '#fc8d59',  # 0.4 - orange
        '#fee08b',  # 0.5 - yellow (1x, neutral)
        '#d9ef8b',  # 0.6 - yellow-green
        '#66bd63',  # 0.8 - green
        '#1a9850',  # 1.0 - dark green (very fast)
    ]
    positions = [0.0, 0.2, 0.4, 0.5, 0.6, 0.8, 1.0]
    return LinearSegmentedColormap.from_list('speedup', list(zip(positions, colors)))

def format_speedup(val):
    """Format speedup value for heatmap annotation."""
    if pd.isna(val) or val == 0:
        return ''
    # Match tmp2.py: 2 decimals if <1, 1 decimal otherwise
    if val < 1:
        return f'{val:.2f}'
    return f'{val:.1f}'

def load_csv(path):
    """Load benchmark CSV."""
    with open(path) as f:
        lines = [l for l in f if not l.startswith('Running:')]
    from io import StringIO
    return pd.read_csv(StringIO(''.join(lines)))

def parse_size(s):
    """Parse size string (e.g., '1KB', '32KB', '1MB') to bytes."""
    s = s.strip().upper()
    if s.endswith('MB'):
        return int(float(s[:-2]) * 1024 * 1024)
    elif s.endswith('KB'):
        return int(float(s[:-2]) * 1024)
    elif s.endswith('B'):
        return int(s[:-1])
    return int(s)

def format_size(size):
    """Format bytes as KB/MB."""
    if size >= 1024 * 1024:
        return f"{size / (1024 * 1024):.0f}MB"
    elif size >= 1024:
        return f"{size / 1024:.0f}KB"
    return f"{size}B"

def find_closest_size(requested, available):
    """Find closest available size to requested."""
    if requested in available:
        return requested
    return min(available, key=lambda x: abs(x - requested))

def get_time(df, pattern, engine, size):
    """Get time_ns for pattern/engine/size."""
    row = df[(df['Pattern'] == pattern) & (df['Engine'] == engine) & (df['Input_Size'] == size)]
    return row['Time_ns'].values[0] if len(row) > 0 else None

def get_pattern_label(pattern):
    """Convert pattern name to readable label."""
    name = pattern.split('/')[-1]
    return name.replace('_', ' ').title()

def print_results(df, size=None):
    """Print benchmark results table."""
    available = [e for e in ENGINES if e in df['Engine'].unique()]
    all_sizes = sorted(df['Input_Size'].unique())
    if size is None:
        size = find_closest_size(1024, all_sizes)
    
    print(f"\n{'Pattern':<30} {'Size':>6}", end='')
    for eng in available:
        print(f" {eng:>11}", end='')
    print(" (ns)")
    
    for pattern in df['Pattern'].unique():
        print(f"{pattern:<30} {format_size(size):>6}", end='')
        for eng in available:
            t = get_time(df, pattern, eng, size)
            print(f" {t:>11.0f}" if t else f" {'-':>11}", end='')
        print()

def print_speedups(df, size=None):
    """Print CTRE-SIMD speedup vs others."""
    available = [e for e in ENGINES if e in df['Engine'].unique() and e != 'CTRE-SIMD']
    all_sizes = sorted(df['Input_Size'].unique())
    if size is None:
        size = find_closest_size(1024, all_sizes)
    
    print(f"\n{'Pattern':<30} {'Size':>6}", end='')
    for eng in available:
        print(f" {eng:>11}", end='')
    print(" (speedup)")
    
    all_speedups = {e: [] for e in available}
    
    for pattern in df['Pattern'].unique():
        simd = get_time(df, pattern, 'CTRE-SIMD', size)
        if not simd or simd <= 0:
            continue
        
        print(f"{pattern:<30} {format_size(size):>6}", end='')
        for eng in available:
            t = get_time(df, pattern, eng, size)
            if t and t > 0:
                speedup = t / simd
                all_speedups[eng].append(speedup)
                print(f" {speedup:>10.1f}x", end='')
            else:
                print(f" {'-':>11}", end='')
        print()
    
    print(f"\n{'Median':<30} {'':>6}", end='')
    for eng in available:
        if all_speedups[eng]:
            print(f" {np.median(all_speedups[eng]):>10.1f}x", end='')
        else:
            print(f" {'-':>11}", end='')
    print()

def print_summary(df, size=None):
    """Print summary statistics for given size."""
    all_sizes = sorted(df['Input_Size'].unique())
    if size is None:
        size = find_closest_size(1024, all_sizes)
    
    wins = {e: 0 for e in df['Engine'].unique()}
    total = 0
    for pattern in df['Pattern'].unique():
        pdf = df[(df['Pattern'] == pattern) & (df['Input_Size'] == size)]
        if len(pdf) > 1:
            wins[pdf.loc[pdf['Time_ns'].idxmin(), 'Engine']] += 1
            total += 1
    
    print(f"\nWins at {format_size(size)} (n={total}):")
    for eng, count in sorted(wins.items(), key=lambda x: -x[1]):
        if count > 0:
            print(f"  {eng:<15} {count:>4} ({100 * count / total:.1f}%)")

def plot_comparison(df, output_path, size=None):
    """Bar chart comparing all engines."""
    setup_style()
    patterns = list(df['Pattern'].unique())
    all_sizes = sorted(df['Input_Size'].unique())
    if size is None:
        size = find_closest_size(1024, all_sizes)
    
    df_size = df[df['Input_Size'] == size]
    engines = [e for e in ENGINES if e in df_size['Engine'].unique()]
    
    fig, ax = plt.subplots(figsize=(12, 6))
    x = np.arange(len(patterns))
    width = PLOT_PARAMS['bar_width']
    
    for i, engine in enumerate(engines):
        times = [get_time(df_size, p, engine, size) or 0 for p in patterns]
        ax.bar(x + (i - len(engines) / 2 + 0.5) * width, times, width,
               label=engine, color=ENGINE_COLORS.get(engine, '#333'),
               edgecolor=PLOT_PARAMS['bar_edge_color'],
               linewidth=PLOT_PARAMS['bar_edge_width'], zorder=3)
    
    ax.set_xlabel('Pattern')
    ax.set_ylabel('Matching Time (ns)')
    ax.set_title(f'Performance Comparison ({format_size(size)} input)')
    ax.set_xticks(x)
    ax.set_xticklabels([get_pattern_label(p) for p in patterns], rotation=30, ha='right')
    ax.set_yscale('log')
    ax.legend(loc='upper right', fontsize=8)
    ax.set_axisbelow(True)
    
    plt.tight_layout()
    fig.savefig(output_path, dpi=PLOT_PARAMS['save_dpi'], bbox_inches='tight', facecolor='white')
    plt.close(fig)
    print(f"Saved {output_path}")

def plot_speedup(df, output_path, size=None):
    """Speedup bar chart vs CTRE-SIMD."""
    setup_style()
    patterns = list(df['Pattern'].unique())
    all_sizes = sorted(df['Input_Size'].unique())
    if size is None:
        size = find_closest_size(1024, all_sizes)
    
    engines = [e for e in ['CTRE-Scalar', 'CTRE', 'RE2', 'PCRE2', 'Hyperscan', 'std::regex']
               if e in df['Engine'].unique()]
    
    fig, ax = plt.subplots(figsize=(12, 6))
    x = np.arange(len(patterns))
    width = PLOT_PARAMS['bar_width']
    
    for i, engine in enumerate(engines):
        speedups = []
        for p in patterns:
            simd = get_time(df, p, 'CTRE-SIMD', size)
            other = get_time(df, p, engine, size)
            speedups.append(other / simd if simd and other and simd > 0 else 0)
        ax.bar(x + (i - len(engines) / 2 + 0.5) * width, speedups, width,
               label=engine, color=ENGINE_COLORS.get(engine, '#333'),
               edgecolor=PLOT_PARAMS['bar_edge_color'],
               linewidth=PLOT_PARAMS['bar_edge_width'], zorder=3)
    
    ax.axhline(y=1.0, color='#333333', linestyle='-', linewidth=1, zorder=1)
    ax.set_xlabel('Pattern')
    ax.set_ylabel('Speedup vs CTRE-SIMD')
    ax.set_title(f'CTRE-SIMD Speedup ({format_size(size)} input)')
    ax.set_xticks(x)
    ax.set_xticklabels([get_pattern_label(p) for p in patterns], rotation=30, ha='right')
    ax.legend(loc='upper right', fontsize=8)
    ax.set_axisbelow(True)
    
    plt.tight_layout()
    fig.savefig(output_path, dpi=PLOT_PARAMS['save_dpi'], bbox_inches='tight', facecolor='white')
    plt.close(fig)
    print(f"Saved {output_path}")

def plot_heatmap(df, output_path, size=None, baseline='CTRE', vmin=0.1, vmax=10.0):
    """Publication-quality speedup heatmap using seaborn."""
    setup_style()
    
    all_sizes = sorted(df['Input_Size'].unique())
    if size is None:
        size = find_closest_size(1024, all_sizes)
    
    size_data = df[df['Input_Size'] == size]
    patterns = list(df['Pattern'].unique())
    
    # Engines to compare (excluding baseline)
    engines = [e for e in ['CTRE-SIMD', 'CTRE-Scalar', 'RE2', 'PCRE2', 'Hyperscan', 'std::regex']
               if e in df['Engine'].unique() and e != baseline]
    
    # Build speedup matrix: engines (rows) x patterns (columns)
    speedup_data = {}
    valid_patterns = []
    
    for pat in patterns:
        pat_data = size_data[size_data['Pattern'] == pat]
        baseline_row = pat_data[pat_data['Engine'] == baseline]['Time_ns'].values
        if len(baseline_row) == 0 or baseline_row[0] <= 0:
            continue
        baseline_time = baseline_row[0]
        pat_label = get_pattern_label(pat)
        valid_patterns.append(pat_label)
        
        for eng in engines:
            speedup_data.setdefault(eng, {})
            eng_time = pat_data[pat_data['Engine'] == eng]['Time_ns'].values
            if len(eng_time) > 0 and eng_time[0] > 0:
                speedup_data[eng][pat_label] = baseline_time / eng_time[0]
            else:
                speedup_data[eng][pat_label] = np.nan
    
    if not valid_patterns:
        print(f"No valid data for heatmap at size {format_size(size)}")
        return
    
    # Create DataFrame for heatmap
    heatmap_df = pd.DataFrame(speedup_data).T.reindex(engines)[valid_patterns]
    
    # Create annotation matrix
    annot = heatmap_df.copy().astype(object)
    for col in heatmap_df.columns:
        for idx in heatmap_df.index:
            annot.loc[idx, col] = format_speedup(heatmap_df.loc[idx, col])
    
    # Create figure with dynamic sizing
    fig_width = len(valid_patterns) * 0.8 + 2
    fig_height = len(engines) * 0.6 + 1.5
    fig, ax = plt.subplots(figsize=(fig_width, fig_height))
    
    # Fixed bounds for consistent colors across all heatmaps
    # Use TwoSlopeNorm: 0.01 (very slow) -> 1.0 (same) -> 100 (very fast)
    from matplotlib.colors import TwoSlopeNorm
    norm = TwoSlopeNorm(vmin=0.01, vcenter=1.0, vmax=100.0)
    
    sns.heatmap(heatmap_df, ax=ax, annot=annot, fmt='',
                cmap=get_heatmap_cmap(), norm=norm,
                linewidths=PLOT_PARAMS['heatmap_linewidth'],
                linecolor=PLOT_PARAMS['heatmap_linecolor'],
                cbar_kws={'label': f'Speedup vs {baseline}', 'shrink': 0.8},
                annot_kws={'size': 10, 'weight': 'medium'})
    
    ax.set_xlabel('')
    ax.set_ylabel('')
    ax.set_title(f'Speedup vs {baseline} ({format_size(size)} input)')
    plt.xticks(rotation=40, ha='right')
    plt.yticks(rotation=0)
    
    plt.tight_layout()
    fig.savefig(output_path, dpi=PLOT_PARAMS['save_dpi'], bbox_inches='tight', facecolor='white')
    plt.close(fig)
    print(f"Saved {output_path}")

def plot_scaling(df, output_path, pattern=None):
    """Performance scaling line plot."""
    setup_style()
    if pattern is None:
        pattern = df['Pattern'].iloc[0]
    
    pdf = df[df['Pattern'] == pattern]
    engines = [e for e in ENGINES if e in pdf['Engine'].unique()]
    sizes = sorted(pdf['Input_Size'].unique())
    
    fig, ax = plt.subplots(figsize=(10, 6))
    
    markers = ['o', 's', '^', 'D', 'v', 'p', 'h']
    for i, engine in enumerate(engines):
        edf = pdf[pdf['Engine'] == engine].sort_values('Input_Size')
        ax.plot(edf['Input_Size'], edf['Time_ns'],
                marker=markers[i % len(markers)], linestyle='-',
                color=ENGINE_COLORS.get(engine, '#333'),
                linewidth=2, markersize=6, label=engine)
    
    ax.set_xlabel('Input Size')
    ax.set_ylabel('Matching Time (ns)')
    ax.set_title(f'Performance Scaling: {get_pattern_label(pattern)}')
    ax.set_xscale('log', base=2)
    ax.set_yscale('log')
    ax.set_xticks(sizes)
    ax.set_xticklabels([format_size(s) for s in sizes])
    ax.legend(loc='upper left', frameon=True)
    
    plt.tight_layout()
    fig.savefig(output_path, dpi=PLOT_PARAMS['save_dpi'], bbox_inches='tight', facecolor='white')
    plt.close(fig)
    print(f"Saved {output_path}")

def speedup_to_color(s):
    """Map speedup value to color: red family (<1.0), green family (>=1.0)."""
    import math
    if s < 0.99:  # Use 0.99 to handle floating point issues
        # Red family for slower than baseline
        if s <= 0.1:
            return '#8b0000'  # dark red
        elif s <= 0.3:
            return '#d73027'  # red
        elif s <= 0.6:
            return '#fc8d59'  # orange-red
        else:
            return '#fdae61'  # light orange-red
    else:
        # Green family for faster than baseline
        if s >= 50:
            return '#1a9850'  # dark green
        elif s >= 10:
            return '#66bd63'  # green
        elif s >= 2:
            return '#a6d96a'  # light green
        else:
            return '#d9ef8b'  # yellow-green

def plot_speedup_bars(df, output_path, size=None):
    """Speedup bar chart with annotations (CTRE-SIMD vs baseline)."""
    setup_style()
    patterns = list(df['Pattern'].unique())
    all_sizes = sorted(df['Input_Size'].unique())
    if size is None:
        size = find_closest_size(1024, all_sizes)
    
    speedups = []
    for pattern in patterns:
        simd = get_time(df, pattern, 'CTRE-SIMD', size)
        baseline = get_time(df, pattern, 'CTRE', size) or get_time(df, pattern, 'CTRE-Scalar', size)
        speedups.append(baseline / simd if simd and baseline and simd > 0 else 1.0)
    
    fig, ax = plt.subplots(figsize=(10, 6))
    x = np.arange(len(patterns))
    colors = [speedup_to_color(s) for s in speedups]
    bars = ax.bar(x, speedups, color=colors,
                  edgecolor=PLOT_PARAMS['bar_edge_color'],
                  linewidth=PLOT_PARAMS['bar_edge_width'])
    ax.axhline(y=1.0, color='#666', linestyle='--', linewidth=1, alpha=0.7)
    
    for bar, s in zip(bars, speedups):
        ax.annotate(f'{s:.2f}×', xy=(bar.get_x() + bar.get_width() / 2, bar.get_height()),
                    xytext=(0, 3), textcoords='offset points', ha='center', va='bottom',
                    fontsize=8, fontweight='bold' if s >= 2 else 'normal')
    
    ax.set_xlabel('Pattern')
    ax.set_ylabel('Speedup')
    ax.set_title(f'CTRE-SIMD Speedup vs Baseline ({format_size(size)} input)')
    ax.set_xticks(x)
    ax.set_xticklabels([get_pattern_label(p) for p in patterns], rotation=30, ha='right')
    ax.set_ylim(bottom=0)
    ax.set_axisbelow(True)
    
    plt.tight_layout()
    fig.savefig(output_path, dpi=PLOT_PARAMS['save_dpi'], bbox_inches='tight', facecolor='white')
    plt.close(fig)
    print(f"Saved {output_path}")

def run_for_size(df, size, out_dir, generate_plots):
    """Run analysis for a single size."""
    print_results(df, size)
    print_speedups(df, size)
    print_summary(df, size)
    
    if generate_plots and out_dir:
        out = Path(out_dir)
        out.mkdir(exist_ok=True)
        suffix = format_size(size).lower()
        plot_comparison(df, out / f'comparison_{suffix}.png', size)
        plot_speedup(df, out / f'speedup_{suffix}.png', size)
        plot_speedup_bars(df, out / f'speedup_bars_{suffix}.png', size)
        plot_heatmap(df, out / f'heatmap_{suffix}.png', size)

def main():
    parser = argparse.ArgumentParser(
        description='Analyze benchmark results',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='Examples:\n'
               '  analyze.py results.csv                  # Default 1KB\n'
               '  analyze.py results.csv -s 32KB          # Use 32KB\n'
               '  analyze.py results.csv --all            # All sizes\n'
               '  analyze.py results.csv --list-sizes     # Show available sizes'
    )
    parser.add_argument('csv', help='Input CSV file')
    parser.add_argument('-s', '--size', default='1KB', help='Input size for tables (default: 1KB)')
    parser.add_argument('-o', '--output', default='plots', help='Output directory for plots')
    parser.add_argument('--all', action='store_true', help='Run for all available sizes')
    parser.add_argument('--list-sizes', action='store_true', help='List available sizes and exit')
    parser.add_argument('--no-plots', action='store_true', help='Skip plot generation')
    args = parser.parse_args()
    
    df = load_csv(args.csv)
    all_sizes = sorted(df['Input_Size'].unique())
    
    if args.list_sizes:
        print("Available sizes:", ', '.join(format_size(s) for s in all_sizes))
        return
    
    print(f"Loaded {len(df)} rows")
    
    if args.all:
        for size in all_sizes:
            print(f"\n{'=' * 60}")
            print(f"SIZE: {format_size(size)}")
            print('=' * 60)
            run_for_size(df, size, args.output if not args.no_plots else None, not args.no_plots)
        
        if not args.no_plots:
            out = Path(args.output)
            out.mkdir(exist_ok=True)
            plot_scaling(df, out / 'scaling_all_sizes.png')
    else:
        requested = parse_size(args.size)
        size = find_closest_size(requested, all_sizes)
        if size != requested:
            print(f"Note: {args.size} not available, using {format_size(size)}")
            print(f"Available: {', '.join(format_size(s) for s in all_sizes)}")
        
        print_results(df, size)
        print_speedups(df, size)
        print_summary(df, size)
        
        if not args.no_plots:
            out = Path(args.output)
            out.mkdir(exist_ok=True)
            suffix = format_size(size).lower()
            plot_comparison(df, out / f'comparison_{suffix}.png', size)
            plot_speedup(df, out / f'speedup_{suffix}.png', size)
            plot_speedup_bars(df, out / f'speedup_bars_{suffix}.png', size)
            plot_heatmap(df, out / f'heatmap_{suffix}.png', size)
            plot_scaling(df, out / 'scaling_all_sizes.png')

if __name__ == '__main__':
    main()
