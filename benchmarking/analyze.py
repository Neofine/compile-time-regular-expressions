#!/usr/bin/env python3
"""Analyze benchmark CSV results and generate blog-quality plots.

Styled for readability and a polished blog look (not paper/journal).
"""

import argparse
import sys
from pathlib import Path

try:
    import pandas as pd
    import matplotlib.pyplot as plt
    import matplotlib as mpl
    import numpy as np
    import seaborn as sns
    from matplotlib.patches import FancyBboxPatch
except ImportError:
    print("Install: pip install pandas matplotlib numpy seaborn")
    sys.exit(1)

# Engine order and palette — modern, distinct, blog-friendly (not default academic)
ENGINES = ['CTRE-SIMD', 'CTRE-Scalar', 'CTRE', 'RE2', 'PCRE2', 'Hyperscan', 'std::regex']
ENGINE_COLORS = {
    'CTRE-SIMD': '#0ea5e9',   # sky-500 (hero)
    'CTRE-Scalar': '#38bdf8', # sky-400
    'CTRE': '#7dd3fc',        # sky-300
    'RE2': '#f43f5e',         # rose-500
    'PCRE2': '#a78bfa',       # violet-400
    'Hyperscan': '#fbbf24',   # amber-400
    'std::regex': '#94a3b8',  # slate-400
}

# Plot parameters
PLOT_PARAMS = {
    'bar_width': 0.15,
    'bar_edge_color': 'white',
    'bar_edge_width': 0.8,
    'bar_rounding_size': 0.02,
    'heatmap_linewidth': 1.5,
    'heatmap_linecolor': 'white',
    'save_dpi': 300,
    # Visible: plot area as a light "card" so the chart reads as a defined block
    'axes_facecolor': '#f1f5f9',
    'figure_facecolor': 'white',
}

def _round_bar_patches(ax):
    """Replace rectangular bar patches with rounded-top boxes for a softer look."""
    rounding = PLOT_PARAMS.get('bar_rounding_size', 0.02)
    new_patches = []
    for patch in reversed(ax.patches):
        bb = patch.get_bbox()
        w, h = abs(bb.width), abs(bb.height)
        if w < 1e-6 or h < 1e-6:
            continue
        fc = patch.get_facecolor()
        ec = patch.get_edgecolor()
        lw = patch.get_linewidth()
        z = patch.get_zorder()
        r = FancyBboxPatch(
            (bb.xmin, bb.ymin), w, h,
            boxstyle=f"round,pad=0,rounding_size={rounding}",
            facecolor=fc, edgecolor=ec, linewidth=lw, zorder=z,
            transform=ax.transData,
        )
        patch.remove()
        new_patches.append(r)
    for p in new_patches:
        ax.add_patch(p)

def setup_style():
    """Blog-style: clean spines, light plot-area background so the chart reads as a card."""
    plt.style.use('seaborn-v0_8-whitegrid')
    mpl.rcParams.update({
        'font.family': 'sans-serif',
        'font.sans-serif': ['DejaVu Sans', 'Liberation Sans', 'Arial', 'sans-serif'],
        'font.size': 11,
        'axes.titlesize': 14,
        'axes.titleweight': 'bold',
        'axes.labelsize': 11,
        'xtick.labelsize': 10,
        'ytick.labelsize': 10,
        'legend.fontsize': 9,
        'figure.dpi': 100,
        'figure.facecolor': PLOT_PARAMS['figure_facecolor'],
        'axes.facecolor': PLOT_PARAMS['axes_facecolor'],
        'savefig.dpi': PLOT_PARAMS['save_dpi'],
        'savefig.bbox': 'tight',
        'savefig.facecolor': 'white',
        'axes.spines.top': False,
        'axes.spines.right': False,
        'axes.linewidth': 1.0,
        'axes.edgecolor': '#334155',
        'axes.grid': True,
        'axes.axisbelow': True,
        'grid.alpha': 0.35,
        'grid.color': '#cbd5e1',
        'legend.frameon': True,
        'legend.framealpha': 0.98,
        'legend.edgecolor': '#e2e8f0',
        'xtick.major.size': 5,
        'ytick.major.size': 5,
        'xtick.minor.visible': False,
        'ytick.minor.visible': False,
    })

def get_heatmap_cmap():
    """Diverging: slow (rose/red) -> 1x (neutral) -> fast (emerald). Blog-friendly."""
    from matplotlib.colors import LinearSegmentedColormap
    colors = [
        '#be123c',  # rose-700
        '#e11d48',  # rose-600
        '#fb7185',  # rose-400
        '#fde68a',  # amber-200 (neutral)
        '#86efac',  # green-300
        '#22c55e',  # green-500
        '#15803d',  # green-700
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
               label=engine, color=ENGINE_COLORS.get(engine, '#64748b'),
               edgecolor=PLOT_PARAMS['bar_edge_color'],
               linewidth=PLOT_PARAMS['bar_edge_width'], zorder=3)
    _round_bar_patches(ax)
    ax.set_xlabel('Pattern')
    ax.set_ylabel('Matching Time (ns)')
    ax.set_title(f'Performance Comparison ({format_size(size)} input)')
    ax.set_xticks(x)
    ax.set_xticklabels([get_pattern_label(p) for p in patterns], rotation=30, ha='right')
    ax.set_yscale('log')
    ax.legend(loc='upper right', fontsize=9, framealpha=0.98, edgecolor='#e2e8f0')
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
               label=engine, color=ENGINE_COLORS.get(engine, '#64748b'),
               edgecolor=PLOT_PARAMS['bar_edge_color'],
               linewidth=PLOT_PARAMS['bar_edge_width'], zorder=3)
    _round_bar_patches(ax)
    ax.axhline(y=1.0, color='#64748b', linestyle='-', linewidth=1.2, alpha=0.9, zorder=1)
    ax.set_xlabel('Pattern')
    ax.set_ylabel('Speedup vs CTRE-SIMD')
    ax.set_title(f'CTRE-SIMD Speedup ({format_size(size)} input)')
    ax.set_xticks(x)
    ax.set_xticklabels([get_pattern_label(p) for p in patterns], rotation=30, ha='right')
    ax.legend(loc='upper right', fontsize=9, framealpha=0.98, edgecolor='#e2e8f0')
    ax.set_axisbelow(True)
    plt.tight_layout()
    fig.savefig(output_path, dpi=PLOT_PARAMS['save_dpi'], bbox_inches='tight', facecolor='white')
    plt.close(fig)
    print(f"Saved {output_path}")

def plot_heatmap(df, output_path, size=None, baseline=None, vmin=0.1, vmax=10.0):
    """Publication-quality speedup heatmap using seaborn."""
    setup_style()
    all_sizes = sorted(df['Input_Size'].unique())
    if size is None:
        size = find_closest_size(1024, all_sizes)
    size_data = df[df['Input_Size'] == size]
    patterns = list(df['Pattern'].unique())
    available = set(df['Engine'].unique())
    candidates = (baseline,) if baseline else ('CTRE', 'CTRE-Scalar', 'CTRE-SIMD')
    baseline = next((b for b in candidates if b in available), None)
    if not baseline:
        print(f"No valid baseline for heatmap at size {format_size(size)}")
        return
    engines = [e for e in ['CTRE-SIMD', 'CTRE-Scalar', 'CTRE', 'RE2', 'PCRE2', 'Hyperscan', 'std::regex']
               if e in available and e != baseline]
    
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
                cbar_kws={
                    'label': f'Speedup vs {baseline}',
                    'shrink': 0.75,
                    'aspect': 25,
                    'pad': 0.02,
                },
                annot_kws={'size': 11, 'weight': 'medium', 'ha': 'center', 'va': 'center'})
    if len(ax.figure.axes) > 1:
        cax = ax.figure.axes[-1]
        cax.tick_params(labelsize=9, length=3, width=0.8)
        cax.set_ylabel(cax.get_ylabel(), fontsize=11, fontweight='medium')
        # Subtle border around colorbar (Colorbar is attached to the collection in some backends)
        try:
            cb = getattr(ax.collections[0], 'colorbar', None)
            if cb is not None and hasattr(cb, 'outline') and cb.outline is not None:
                cb.outline.set_edgecolor('#e2e8f0')
                cb.outline.set_linewidth(0.8)
        except (IndexError, AttributeError):
            pass
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
    """Performance scaling line plot — thick lines, clear markers, pro legend."""
    setup_style()
    if pattern is None:
        pattern = df['Pattern'].iloc[0]
    pdf = df[df['Pattern'] == pattern]
    engines = [e for e in ENGINES if e in pdf['Engine'].unique()]
    sizes = sorted(pdf['Input_Size'].unique())
    fig, ax = plt.subplots(figsize=(11, 6))
    markers = ['o', 's', '^', 'D', 'v', 'p', 'h']
    for i, engine in enumerate(engines):
        edf = pdf[pdf['Engine'] == engine].sort_values('Input_Size')
        ax.plot(edf['Input_Size'], edf['Time_ns'],
                 marker=markers[i % len(markers)], linestyle='-',
                 color=ENGINE_COLORS.get(engine, '#64748b'),
                 linewidth=2.5, markersize=8, markeredgecolor='white', markeredgewidth=0.8,
                 label=engine)
    ax.set_xlabel('Input Size')
    ax.set_ylabel('Matching Time (ns)')
    ax.set_title(f'Performance Scaling: {get_pattern_label(pattern)}')
    ax.set_xscale('log', base=2)
    ax.set_yscale('log')
    ax.set_xticks(sizes)
    ax.set_xticklabels([format_size(s) for s in sizes])
    ncol = 2 if len(engines) > 4 else 1
    ax.legend(loc='upper left', frameon=True, fancybox=True, shadow=True,
              fontsize=9, framealpha=0.98, edgecolor='#e2e8f0', ncol=ncol)
    plt.tight_layout()
    fig.savefig(output_path, dpi=PLOT_PARAMS['save_dpi'], bbox_inches='tight', facecolor='white')
    plt.close(fig)
    print(f"Saved {output_path}")

def speedup_to_color(s):
    """Map speedup to color: rose (<1x), amber (1x), emerald (>1x). Matches heatmap."""
    if s < 0.99:
        if s <= 0.2:
            return '#be123c'   # rose-700
        elif s <= 0.5:
            return '#e11d48'   # rose-600
        else:
            return '#fb7185'   # rose-400
    else:
        if s >= 10:
            return '#15803d'   # green-700
        elif s >= 3:
            return '#22c55e'   # green-500
        elif s >= 1.5:
            return '#86efac'   # green-300
        else:
            return '#fde68a'   # amber-200 (neutral)

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
    ax.bar(x, speedups, color=colors,
           edgecolor=PLOT_PARAMS['bar_edge_color'],
           linewidth=PLOT_PARAMS['bar_edge_width'])
    _round_bar_patches(ax)
    ax.axhline(y=1.0, color='#64748b', linestyle='--', linewidth=1.2, alpha=0.8)
    # Headroom so labels above bars don't clip (PolicyViz: labels at end of bars)
    y_max = max(speedups) if speedups else 1
    ax.set_ylim(0, y_max * 1.14)
    try:
        from matplotlib.patheffects import withStroke
        path_effects = [withStroke(linewidth=1.5, foreground='white')]
    except ImportError:
        path_effects = []
    for bar, s in zip(reversed(list(ax.patches)), speedups):
        cx = bar.get_x() + bar.get_width() / 2
        top = bar.get_height()
        weight = 'bold' if s >= 2 or s < 0.5 else 'medium'
        ax.annotate(f'{s:.2f}×', xy=(cx, top), xytext=(0, 5), textcoords='offset points',
                    ha='center', va='bottom', fontsize=9, fontweight=weight,
                    color='#1e293b', path_effects=path_effects)
    ax.set_xlabel('Pattern')
    ax.set_ylabel('Speedup')
    ax.set_title(f'CTRE-SIMD Speedup vs Baseline ({format_size(size)} input)')
    ax.set_xticks(x)
    ax.set_xticklabels([get_pattern_label(p) for p in patterns], rotation=30, ha='right')
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
