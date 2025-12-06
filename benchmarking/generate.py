#!/usr/bin/env python3
"""
Thesis Plot Generator

Main entry point for generating all benchmark visualizations.

Usage:
    python generate.py                    # Generate all plots
    python generate.py --category simple  # Specific category
    python generate.py --list             # List categories
    python generate.py --clean            # Clean and regenerate

Output:
    output/figures/           # All generated plots
    output/figures/simple/    # Simple pattern plots
    output/figures/complex/   # Complex pattern plots
    ...
"""

import argparse
import logging
import sys
import shutil
from pathlib import Path
import matplotlib.pyplot as plt

# Setup paths
SCRIPT_DIR = Path(__file__).parent.resolve()
sys.path.insert(0, str(SCRIPT_DIR / 'plotting'))

from plotting.config import setup_style, ENGINE_ORDER, format_size, get_pattern_label
from plotting.data import load_benchmark_results, load_compile_overhead, load_compile_time, merge_simd_baseline
from plotting.figures import (
    TimeSeriesPlot,
    SpeedupPlot,
    BarComparison,
    ScalingBars,
    SpeedupHeatmap,
    WorstCaseHeatmap,
    CompileOverheadPlot,
    SIMDOverheadPlot,
)

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(message)s')
logger = logging.getLogger(__name__)

# ============================================================================
# Configuration
# ============================================================================

DATA_DIR = SCRIPT_DIR / 'output'
OUTPUT_DIR = SCRIPT_DIR / 'output' / 'figures'

# Legacy data locations (for backwards compatibility)
LEGACY_DATA_DIR = SCRIPT_DIR / 'output' / 'thesis'

CATEGORIES = {
    'simple': {
        'prefix': 'Simple/',
        'title': 'Simple Patterns',
        'heatmap_size': 1024,  # 1024 bytes (2^10)
    },
    'complex': {
        'prefix': 'Complex/',
        'title': 'Complex Patterns',
        'heatmap_size': 1024,
    },
    'scaling': {
        'prefix': 'Scaling/',
        'title': 'Pattern Scaling',
        'heatmap_size': 1024,
    },
    'realworld': {
        'prefix': 'RealWorld/',
        'title': 'Real-World Workloads',
        'heatmap_size': 1024,
    },
    'nomatch': {
        'prefix': 'NonMatch/',
        'title': 'Non-Matching Inputs',
        'heatmap_size': 1024,
    },
    'small': {
        'prefix': 'Small/',
        'title': 'Small Inputs (SIMD Overhead)',
        'heatmap_size': 16,
        'log_x': False,  # Linear scale for 1-16 byte inputs
    },
    'large': {
        'prefix': 'Large/',
        'title': 'Large Inputs (Throughput)',
        'heatmap_size': 65536,
    },
    'fallback': {
        'prefix': 'Fallback/',
        'title': 'Fallback Patterns (No SIMD)',
        'heatmap_size': 1024,
    },
    'instantiation': {
        'prefix': 'Instantiation/',
        'title': 'Regex Instantiation Time',
        'heatmap_size': None,  # No heatmap - single size
        'custom_plot': 'instantiation',
    },
    'arm': {
        'prefix': 'arm/',
        'title': 'ARM Matching Patterns',
        'heatmap_size': 256,
        'skip_heatmap': True,
    },
    'arm_nomatch': {
        'prefix': 'arm_nomatch/',
        'title': 'ARM Non-Matching (Early Rejection)',
        'heatmap_size': 256,
        'skip_heatmap': True,
    },
}

# ============================================================================
# Plot Generators
# ============================================================================

def generate_time_series(data, output_dir, config):
    """Generate time series plots for each pattern."""
    prefix = config['prefix']
    patterns = [p for p in data.patterns if p.startswith(prefix)]
    log_x = config.get('log_x', True)  # Default to log scale, but can be disabled

    for pattern in patterns:
        name = pattern.split('/')[-1]
        label = get_pattern_label(name)  # Show regex if short
        plot = TimeSeriesPlot(title=label, log_x=log_x)
        plot.plot(data, pattern)
        plot.save(output_dir / f'{name}_time.png')

def generate_heatmap(data, output_dir, config, size=1024):
    """Generate speedup heatmap."""
    prefix = config['prefix']
    title = f"{config['title']} — Speedup ({format_size(size)})"

    plot = SpeedupHeatmap(title=title)
    plot.plot(data, size=size, pattern_prefix=prefix)
    plot.save(output_dir / 'heatmap.png')

def generate_bar_chart(data, output_dir, config, size=1024):
    """Generate bar comparison chart."""
    prefix = config['prefix']
    patterns = [p for p in data.patterns if p.startswith(prefix)]
    labels = [get_pattern_label(p.split('/')[-1]) for p in patterns]  # Show regex if short

    title = f"{config['title']} — Comparison ({format_size(size)})"

    plot = BarComparison(title=title)
    plot.plot(data, patterns, labels, size=size)
    plot.save(output_dir / 'bar_comparison.png')

def generate_speedup_bars(data, output_dir, config, size=1024):
    """Generate speedup bar chart (for ARM - CTRE-Scalar vs CTRE comparison)."""
    import numpy as np

    prefix = config['prefix']
    patterns = [p for p in data.patterns if p.startswith(prefix)]
    labels = [get_pattern_label(p.split('/')[-1]) for p in patterns]

    # Calculate speedups (CTRE-Scalar vs original CTRE)
    speedups = []
    for pattern in patterns:
        # Try CTRE-Scalar first, fall back to CTRE-SIMD for backwards compat
        scalar_time = data.get_time(pattern, 'CTRE-Scalar', size) or data.get_time(pattern, 'CTRE-SIMD', size)
        base_time = data.get_time(pattern, 'CTRE', size)
        if scalar_time and base_time and scalar_time > 0:
            speedups.append(base_time / scalar_time)
        else:
            speedups.append(1.0)

    # Create figure
    fig, ax = plt.subplots(figsize=(10, 6))

    x = np.arange(len(patterns))
    colors = ['#2166ac' if s >= 1.0 else '#c51b7d' for s in speedups]

    bars = ax.bar(x, speedups, color=colors, edgecolor='white', linewidth=0.5)

    # Reference line at 1.0
    ax.axhline(y=1.0, color='#666666', linestyle='--', linewidth=1, alpha=0.7)

    # Labels
    ax.set_xlabel('Pattern', fontsize=11)
    ax.set_ylabel('Speedup (baseline / SIMD)', fontsize=11)
    ax.set_title(f"{config['title']} — CTRE-Scalar Speedup ({format_size(size)})", fontsize=12, fontweight='bold')
    ax.set_xticks(x)
    ax.set_xticklabels(labels, rotation=45, ha='right', fontsize=9)

    # Add value labels on bars
    for bar, speedup in zip(bars, speedups):
        height = bar.get_height()
        ax.annotate(f'{speedup:.2f}x',
                    xy=(bar.get_x() + bar.get_width() / 2, height),
                    xytext=(0, 3), textcoords="offset points",
                    ha='center', va='bottom', fontsize=8)

    ax.set_ylim(bottom=0)
    plt.tight_layout()
    fig.savefig(output_dir / 'speedup_bars.png', dpi=150, bbox_inches='tight',
                facecolor='white', edgecolor='none')
    plt.close(fig)

def generate_simd_delta_bars(data, output_dir, categories):
    """Generate bar chart showing max SIMD time delta per input size.

    For each input size (2^4 to 2^15), shows:
    - Green bars: max time saved by SIMD (best case win)
    - Red bars: max time lost by SIMD (worst case loss)
    """
    import numpy as np

    if data is None:
        logger.warning("No data for SIMD delta bars")
        return

    df = data  # data is already a DataFrame

    # Filter patterns from specified categories
    prefixes = [CATEGORIES[cat]['prefix'] for cat in categories if cat in CATEGORIES]
    all_patterns = df['Pattern'].unique()
    patterns = [p for p in all_patterns if any(p.startswith(pre) for pre in prefixes)]

    # Sizes from 2^4 to 2^15
    sizes = [2**i for i in range(4, 16)]

    max_savings = []  # Best case: SIMD saves the most time
    max_losses = []   # Worst case: SIMD loses the most time
    max_savings_pct = []  # Best case as percentage
    max_losses_pct = []   # Worst case as percentage

    for size in sizes:
        best_delta = 0   # Most positive = biggest win
        worst_delta = 0  # Most negative = biggest loss
        best_pct = 0
        worst_pct = 0

        for pattern in patterns:
            pat_data = df[(df['Pattern'] == pattern) & (df['Input_Size'] == size)]

            baseline_row = pat_data[pat_data['Engine'] == 'CTRE']
            simd_row = pat_data[pat_data['Engine'] == 'CTRE-SIMD']

            if len(baseline_row) > 0 and len(simd_row) > 0:
                baseline_ns = baseline_row['Time_ns'].values[0]
                simd_ns = simd_row['Time_ns'].values[0]
                delta = baseline_ns - simd_ns  # Positive = SIMD faster
                pct = (delta / baseline_ns) * 100 if baseline_ns > 0 else 0

                if delta > best_delta:
                    best_delta = delta
                    best_pct = pct
                if delta < worst_delta:
                    worst_delta = delta
                    worst_pct = pct

        max_savings.append(best_delta)
        max_losses.append(worst_delta)
        max_savings_pct.append(best_pct)
        max_losses_pct.append(abs(worst_pct))  # Absolute for display

    # Create dual-panel plot
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))

    x = np.arange(len(sizes))
    width = 0.35

    # For log scale, we need absolute values
    abs_losses = [-v if v < 0 else 0.1 for v in max_losses]  # 0.1 as placeholder for 0
    abs_savings = [v if v > 0 else 0.1 for v in max_savings]

    # LEFT PANEL: Absolute time delta (log scale)
    bars_win1 = ax1.bar(x - width/2, abs_savings, width, label='Max time saved',
                        color='#2166ac', edgecolor='black', linewidth=0.5)
    bars_loss1 = ax1.bar(x + width/2, abs_losses, width, label='Max time lost',
                         color='#c51b7d', edgecolor='black', linewidth=0.5)

    ax1.set_yscale('log')
    ax1.set_xlabel('Input Size', fontsize=10)
    ax1.set_ylabel('Time Delta (ns) — log scale', fontsize=10)
    ax1.set_title('Absolute Time Impact', fontsize=11, fontweight='bold')
    ax1.set_xticks(x)
    ax1.set_xticklabels([format_size(s) for s in sizes], rotation=45, ha='right', fontsize=8)
    ax1.legend(loc='upper left', fontsize=9)
    ax1.spines['top'].set_visible(False)
    ax1.spines['right'].set_visible(False)

    # RIGHT PANEL: Percentage of baseline time
    bars_win2 = ax2.bar(x - width/2, max_savings_pct, width, label='Best speedup %',
                        color='#2166ac', edgecolor='black', linewidth=0.5)
    bars_loss2 = ax2.bar(x + width/2, max_losses_pct, width, label='Worst slowdown %',
                         color='#c51b7d', edgecolor='black', linewidth=0.5)

    ax2.set_xlabel('Input Size', fontsize=10)
    ax2.set_ylabel('Percentage of Baseline Time', fontsize=10)
    ax2.set_title('Relative Impact (%)', fontsize=11, fontweight='bold')
    ax2.set_xticks(x)
    ax2.set_xticklabels([format_size(s) for s in sizes], rotation=45, ha='right', fontsize=8)
    ax2.legend(loc='upper left', fontsize=9)
    ax2.spines['top'].set_visible(False)
    ax2.spines['right'].set_visible(False)

    # Add 100% reference line
    ax2.axhline(y=100, color='gray', linestyle='--', linewidth=0.5, alpha=0.5)
    ax2.text(len(sizes) - 0.5, 102, '100%', fontsize=8, alpha=0.7)

    fig.suptitle('SIMD Impact: Best vs Worst Case (Simple + Complex patterns)',
                 fontsize=12, fontweight='bold', y=1.02)

    plt.tight_layout()
    plt.savefig(output_dir / 'simd_delta_by_size.png', dpi=150, bbox_inches='tight')
    plt.close()

    logger.info(f"  ✓ SIMD delta bars saved to {output_dir / 'simd_delta_by_size.png'}")


def generate_fusion_plot(data, output_dir, config):
    """DEPRECATED - Fusion category removed."""
    pass


def generate_instantiation_plot(df, output_dir, config):
    """Generate bar chart showing regex instantiation time by engine.

    Shows how long each runtime engine takes to compile a regex pattern.
    CTRE is shown as 0 since it compiles at C++ compile time.
    """
    import numpy as np

    prefix = config['prefix']
    patterns = sorted([p for p in df['Pattern'].unique() if p.startswith(prefix)])

    if not patterns:
        logger.warning("No instantiation patterns found")
        return

    # Engines to show
    engines = ['std::regex', 'PCRE2', 'RE2', 'Hyperscan', 'CTRE-SIMD']
    engine_colors = {
        'std::regex': '#666666',
        'PCRE2': '#8073ac',
        'RE2': '#d6604d',
        'Hyperscan': '#f4a582',
        'CTRE-SIMD': '#2166ac',
    }

    # Extract data
    pattern_names = [p.split('/')[-1] for p in patterns]

    fig, ax = plt.subplots(figsize=(12, 6))

    x = np.arange(len(pattern_names))
    width = 0.15

    for i, engine in enumerate(engines):
        times = []
        for pattern in patterns:
            pat_data = df[(df['Pattern'] == pattern) & (df['Engine'] == engine)]
            if len(pat_data) > 0:
                times.append(pat_data['Time_ns'].values[0])
            else:
                times.append(0)

        # Replace 0 with small value for log scale
        times_plot = [max(t, 0.1) for t in times]

        offset = (i - len(engines)/2 + 0.5) * width
        bars = ax.bar(x + offset, times_plot, width, label=engine,
                     color=engine_colors.get(engine, '#999999'),
                     edgecolor='black', linewidth=0.5)

        # Add value labels on bars
        for bar, t in zip(bars, times):
            height = bar.get_height()
            if t == 0:
                label = '0ns'
            elif t >= 1000000:
                label = f'{t/1000000:.1f}ms'
            elif t >= 1000:
                label = f'{t/1000:.0f}μs'
            else:
                label = f'{t:.0f}ns'
            ax.annotate(label,
                       xy=(bar.get_x() + bar.get_width() / 2, height),
                       xytext=(0, 3),
                       textcoords='offset points',
                       ha='center', va='bottom', fontsize=6, rotation=90)

    ax.set_xlabel('Pattern Complexity', fontsize=11)
    ax.set_ylabel('Instantiation Time (ns) — log scale', fontsize=11)
    ax.set_title('Regex Compilation/Instantiation Time\n(CTRE = 0ns at runtime, compiled at C++ compile time)',
                 fontsize=12, fontweight='bold')

    ax.set_xticks(x)
    ax.set_xticklabels(pattern_names, rotation=30, ha='right')
    ax.set_yscale('log')

    ax.legend(loc='upper left', fontsize=9)
    ax.spines['top'].set_visible(False)
    ax.spines['right'].set_visible(False)

    plt.tight_layout()
    plt.savefig(output_dir / 'instantiation_time.png', dpi=150, bbox_inches='tight')
    plt.close()

    logger.info(f"  ✓ Instantiation time chart saved")

def validate_data(data, prefix: str) -> dict:
    """Validate data and return stats."""
    patterns = [p for p in data.patterns if p.startswith(prefix)]
    engines = data.engines

    return {
        'patterns': patterns,
        'engines': engines,
        'has_simd': 'CTRE-SIMD' in engines,
        'has_baseline': 'CTRE' in engines,
        'pattern_count': len(patterns),
        'engine_count': len(engines),
    }

def generate_category(category: str, data_dir: Path, output_dir: Path) -> bool:
    """Generate all plots for a category."""
    config = CATEGORIES.get(category)
    if not config:
        logger.error(f"Unknown category: {category}")
        return False

    logger.info(f"\n{'='*50}")
    logger.info(f"  {config['title']}")
    logger.info(f"{'='*50}")

    # Try loading from new location first, fall back to legacy
    data = load_benchmark_results(data_dir, category)
    if data is None:
        data = load_benchmark_results(LEGACY_DATA_DIR, category)

    if data is None:
        logger.warning(f"  No data found for {category}")
        return False

    # Validate data
    stats = validate_data(data, config['prefix'])
    logger.info(f"  Data: {stats['pattern_count']} patterns, {stats['engine_count']} engines")

    if not stats['has_simd'] or not stats['has_baseline']:
        logger.warning(f"  ⚠ Missing engines: SIMD={stats['has_simd']}, Baseline={stats['has_baseline']}")

    if stats['pattern_count'] == 0:
        logger.warning(f"  ⚠ No patterns found with prefix '{config['prefix']}'")
        return False

    cat_output = output_dir / category
    cat_output.mkdir(parents=True, exist_ok=True)

    # Check if this category has custom plotting
    if config.get('custom_plot'):
        custom_type = config.get('custom_plot')
        try:
            if custom_type == 'instantiation':
                generate_instantiation_plot(data.df, cat_output, config)
            else:
                generate_fusion_plot(data, cat_output, config)
                logger.info("  ✓ Segment complexity chart")
        except Exception as e:
            logger.error(f"  ✗ Custom plot: {e}")
            import traceback
            traceback.print_exc()
        return True

    # Standard plots for other categories
    # Use size 256 for heatmap/bar so std::regex is included
    # (std::regex crashes with stack overflow on larger inputs)
    heatmap_size = config.get('heatmap_size', 256)

    try:
        generate_time_series(data, cat_output, config)
        logger.info("  ✓ Time series")
    except Exception as e:
        logger.error(f"  ✗ Time series: {e}")

    if config.get('skip_heatmap'):
        try:
            generate_speedup_bars(data, cat_output, config, size=heatmap_size)
            logger.info("  ✓ Speedup bars")
        except Exception as e:
            logger.error(f"  ✗ Speedup bars: {e}")
    else:
        try:
            generate_heatmap(data, cat_output, config, size=heatmap_size)
            logger.info("  ✓ Heatmap")
        except Exception as e:
            logger.error(f"  ✗ Heatmap: {e}")

    try:
        generate_bar_chart(data, cat_output, config, size=heatmap_size)
        logger.info("  ✓ Bar chart")
    except Exception as e:
        logger.error(f"  ✗ Bar chart: {e}")

    return True

def generate_compile_overhead(data_dir: Path, output_dir: Path):
    """Generate compilation overhead plots."""
    logger.info(f"\n{'='*50}")
    logger.info("  Compilation Overhead")
    logger.info(f"{'='*50}")

    df = load_compile_overhead(data_dir)
    if df is None:
        df = load_compile_overhead(LEGACY_DATA_DIR)

    if df is None:
        logger.warning("  No compile data found")
        return

    cat_output = output_dir / 'compile_overhead'
    cat_output.mkdir(parents=True, exist_ok=True)

    try:
        plot = CompileOverheadPlot(title='Compilation Overhead')
        plot.plot(df)
        plot.save(cat_output / 'compile_summary.png')
        logger.info("  ✓ Compile summary")
    except Exception as e:
        logger.error(f"  ✗ Compile summary: {e}")

    try:
        plot = SIMDOverheadPlot(title='SIMD Overhead')
        plot.plot(df)
        plot.save(cat_output / 'simd_overhead.png')
        logger.info("  ✓ SIMD overhead")
    except Exception as e:
        logger.error(f"  ✗ SIMD overhead: {e}")

def generate_codesize(data_dir: Path, output_dir: Path):
    """Generate binary size comparison plots."""
    import pandas as pd
    import matplotlib.pyplot as plt
    import numpy as np

    logger.info(f"\n{'='*50}")
    logger.info("  Code Size")
    logger.info(f"{'='*50}")

    csv_path = data_dir / 'codesize.csv'
    if not csv_path.exists():
        logger.warning("  No codesize data found")
        return

    df = pd.read_csv(csv_path)
    cat_output = output_dir / 'codesize'
    cat_output.mkdir(parents=True, exist_ok=True)

    # Check if we have Text_Bytes column (new format)
    size_col = 'Text_Bytes' if 'Text_Bytes' in df.columns else 'Binary_Size_KB'

    try:
        setup_style()
        fig, ax = plt.subplots(figsize=(10, 6))

        patterns = df['Pattern'].unique()
        pattern_labels = [get_pattern_label(p) for p in patterns]  # Show regex
        x = np.arange(len(patterns))
        width = 0.35

        baseline = df[df['Variant'] == 'Baseline'][size_col].values
        simd = df[df['Variant'] == 'SIMD'][size_col].values

        # Convert to KB for display
        baseline_kb = baseline / 1024
        simd_kb = simd / 1024

        bars1 = ax.bar(x - width/2, baseline_kb, width, label='Baseline', color='#92c5de')
        bars2 = ax.bar(x + width/2, simd_kb, width, label='SIMD', color='#2166ac')

        # Add value labels
        for bar in bars1:
            height = bar.get_height()
            ax.annotate(f'{height:.1f}',
                       xy=(bar.get_x() + bar.get_width()/2, height),
                       xytext=(0, 3), textcoords='offset points',
                       ha='center', va='bottom', fontsize=8)
        for bar in bars2:
            height = bar.get_height()
            ax.annotate(f'{height:.1f}',
                       xy=(bar.get_x() + bar.get_width()/2, height),
                       xytext=(0, 3), textcoords='offset points',
                       ha='center', va='bottom', fontsize=8)

        ax.set_xlabel('Pattern')
        ax.set_ylabel('Code Size (KB)')
        ax.set_title('Code Section Size: Baseline vs SIMD')
        ax.set_xticks(x)
        ax.set_xticklabels(pattern_labels, rotation=30, ha='right')
        ax.legend()

        plt.tight_layout()
        fig.savefig(cat_output / 'codesize_comparison.png', dpi=300, bbox_inches='tight')
        plt.close(fig)
        logger.info("  ✓ Codesize comparison")

        # Also generate overhead percentage plot
        fig2, ax2 = plt.subplots(figsize=(10, 6))
        overhead_pct = ((simd - baseline) / baseline) * 100
        colors = ['#2166ac' if o > 0 else '#92c5de' for o in overhead_pct]
        bars = ax2.bar(x, overhead_pct, color=colors)
        ax2.axhline(y=0, color='black', linestyle='-', linewidth=0.5)
        ax2.set_xlabel('Pattern')
        ax2.set_ylabel('SIMD Code Overhead (%)')
        ax2.set_title('SIMD Code Size Overhead vs Baseline')
        ax2.set_xticks(x)
        ax2.set_xticklabels(patterns, rotation=30, ha='right')

        for bar, pct in zip(bars, overhead_pct):
            height = bar.get_height()
            ax2.annotate(f'{pct:.0f}%',
                        xy=(bar.get_x() + bar.get_width()/2, height),
                        xytext=(0, 3 if height >= 0 else -12),
                        textcoords='offset points',
                        ha='center', va='bottom' if height >= 0 else 'top',
                        fontsize=9, fontweight='bold')

        plt.tight_layout()
        fig2.savefig(cat_output / 'simd_overhead.png', dpi=300, bbox_inches='tight')
        plt.close(fig2)
        logger.info("  ✓ SIMD overhead")

    except Exception as e:
        logger.error(f"  ✗ Codesize: {e}")

def generate_compile_time(data_dir: Path, output_dir: Path):
    """Generate compilation time comparison plots."""
    import pandas as pd
    import matplotlib.pyplot as plt
    import numpy as np

    logger.info(f"\n{'='*50}")
    logger.info("  Compilation Time")
    logger.info(f"{'='*50}")

    df = load_compile_time(data_dir)
    if df is None:
        logger.warning("  No compile time data found. Run: bash scripts/measure_compile_time.sh")
        return

    cat_output = output_dir / 'compile_time'
    cat_output.mkdir(parents=True, exist_ok=True)

    try:
        setup_style()

        # Group by category
        categories = df['Category'].unique()

        # Plot 1: Compilation time by category
        fig, ax = plt.subplots(figsize=(12, 6))

        x = np.arange(len(categories))
        width = 0.35

        baseline_times = []
        simd_times = []
        for cat in categories:
            cat_data = df[df['Category'] == cat]
            baseline_times.append(cat_data[cat_data['Variant'] == 'Baseline']['Compile_Time_s'].mean())
            simd_times.append(cat_data[cat_data['Variant'] == 'SIMD']['Compile_Time_s'].mean())

        bars1 = ax.bar(x - width/2, baseline_times, width, label='Baseline', color='#92c5de')
        bars2 = ax.bar(x + width/2, simd_times, width, label='SIMD', color='#2166ac')

        # Add value labels
        for bars in [bars1, bars2]:
            for bar in bars:
                height = bar.get_height()
                ax.annotate(f'{height:.2f}s',
                           xy=(bar.get_x() + bar.get_width()/2, height),
                           xytext=(0, 3), textcoords='offset points',
                           ha='center', va='bottom', fontsize=9)

        ax.set_xlabel('Pattern Category')
        ax.set_ylabel('Average Compile Time (seconds)')
        ax.set_title('Compilation Time: Baseline vs SIMD')
        ax.set_xticks(x)
        ax.set_xticklabels(categories)
        ax.legend()

        plt.tight_layout()
        fig.savefig(cat_output / 'compile_time_by_category.png', dpi=300, bbox_inches='tight')
        plt.close(fig)
        logger.info("  ✓ Compile time by category")

        # Plot 2: Per-pattern compile time
        fig2, ax2 = plt.subplots(figsize=(14, 8))

        patterns = df['Pattern'].unique()
        x2 = np.arange(len(patterns))

        baseline_times_all = []
        simd_times_all = []
        pattern_categories = []
        for pat in patterns:
            pat_data = df[df['Pattern'] == pat]
            baseline = pat_data[pat_data['Variant'] == 'Baseline']['Compile_Time_s'].values
            simd = pat_data[pat_data['Variant'] == 'SIMD']['Compile_Time_s'].values
            baseline_times_all.append(baseline[0] if len(baseline) > 0 else 0)
            simd_times_all.append(simd[0] if len(simd) > 0 else 0)
            cat = pat_data['Category'].values[0] if len(pat_data) > 0 else 'Unknown'
            pattern_categories.append(cat)

        bars1 = ax2.bar(x2 - width/2, baseline_times_all, width, label='Baseline', color='#92c5de')
        bars2 = ax2.bar(x2 + width/2, simd_times_all, width, label='SIMD', color='#2166ac')

        ax2.set_xlabel('Pattern')
        ax2.set_ylabel('Compile Time (seconds)')
        ax2.set_title('Compilation Time per Pattern')
        ax2.set_xticks(x2)
        ax2.set_xticklabels([get_pattern_label(p) for p in patterns], rotation=45, ha='right')
        ax2.legend()

        plt.tight_layout()
        fig2.savefig(cat_output / 'compile_time_per_pattern.png', dpi=300, bbox_inches='tight')
        plt.close(fig2)
        logger.info("  ✓ Compile time per pattern")

        # Plot 3: SIMD compile overhead percentage
        fig3, ax3 = plt.subplots(figsize=(12, 6))

        overhead_pct = []
        for b, s in zip(baseline_times_all, simd_times_all):
            if b > 0:
                overhead_pct.append((s - b) / b * 100)
            else:
                overhead_pct.append(0)

        colors = ['#b2182b' if o > 0 else '#2166ac' for o in overhead_pct]
        bars = ax3.bar(x2, overhead_pct, color=colors)
        ax3.axhline(y=0, color='black', linestyle='-', linewidth=0.5)

        ax3.set_xlabel('Pattern')
        ax3.set_ylabel('SIMD Compile Overhead (%)')
        ax3.set_title('SIMD Compilation Overhead vs Baseline')
        ax3.set_xticks(x2)
        ax3.set_xticklabels([get_pattern_label(p) for p in patterns], rotation=45, ha='right')

        # Add percentage labels
        for bar, pct in zip(bars, overhead_pct):
            height = bar.get_height()
            ax3.annotate(f'{pct:.0f}%',
                        xy=(bar.get_x() + bar.get_width()/2, height),
                        xytext=(0, 3 if height >= 0 else -12),
                        textcoords='offset points',
                        ha='center', va='bottom' if height >= 0 else 'top',
                        fontsize=8, fontweight='bold')

        plt.tight_layout()
        fig3.savefig(cat_output / 'compile_overhead_pct.png', dpi=300, bbox_inches='tight')
        plt.close(fig3)
        logger.info("  ✓ Compile overhead percentage")

    except Exception as e:
        logger.error(f"  ✗ Compile time plots: {e}")
        import traceback
        traceback.print_exc()

# ============================================================================
# Main
# ============================================================================

def main():
    parser = argparse.ArgumentParser(
        description='Generate thesis benchmark plots',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        '--category', '-c',
        choices=list(CATEGORIES.keys()) + ['all', 'compile', 'codesize', 'compiletime'],
        default='all',
        help='Category to generate',
    )
    parser.add_argument(
        '--data-dir', '-d',
        type=Path,
        default=DATA_DIR,
        help='Data directory',
    )
    parser.add_argument(
        '--output-dir', '-o',
        type=Path,
        default=OUTPUT_DIR,
        help='Output directory',
    )
    parser.add_argument(
        '--list', '-l',
        action='store_true',
        help='List categories',
    )
    parser.add_argument(
        '--clean',
        action='store_true',
        help='Clean output before generating',
    )
    parser.add_argument(
        '--delta',
        action='store_true',
        help='Generate SIMD delta bar chart (best/worst case per size)',
    )

    args = parser.parse_args()

    if args.list:
        print("Categories:")
        for cat, cfg in CATEGORIES.items():
            print(f"  {cat}: {cfg['title']}")
        print("  compile: Compilation overhead (legacy)")
        print("  codesize: Binary size comparison")
        print("  compiletime: Compilation time measurement")
        return

    setup_style()

    logger.info("Thesis Plot Generator")
    logger.info(f"Data:   {args.data_dir}")
    logger.info(f"Output: {args.output_dir}")

    if args.clean and args.output_dir.exists():
        shutil.rmtree(args.output_dir)
        logger.info("Cleaned output directory")

    args.output_dir.mkdir(parents=True, exist_ok=True)

    if args.delta:
        # Generate SIMD delta chart for simple + complex
        data = merge_simd_baseline(args.data_dir / 'simd.csv', args.data_dir / 'baseline.csv')
        delta_dir = args.output_dir / 'overview'
        delta_dir.mkdir(parents=True, exist_ok=True)
        generate_simd_delta_bars(data, delta_dir, ['simple', 'complex'])
    elif args.category == 'all':
        for cat in CATEGORIES:
            generate_category(cat, args.data_dir, args.output_dir)
        generate_compile_overhead(args.data_dir, args.output_dir)
        generate_codesize(args.data_dir, args.output_dir)
        generate_compile_time(args.data_dir, args.output_dir)
        # Also generate delta chart
        data = merge_simd_baseline(args.data_dir / 'simd.csv', args.data_dir / 'baseline.csv')
        delta_dir = args.output_dir / 'overview'
        delta_dir.mkdir(parents=True, exist_ok=True)
        generate_simd_delta_bars(data, delta_dir, ['simple', 'complex'])
    elif args.category == 'compile':
        generate_compile_overhead(args.data_dir, args.output_dir)
    elif args.category == 'codesize':
        generate_codesize(args.data_dir, args.output_dir)
    elif args.category == 'compiletime':
        generate_compile_time(args.data_dir, args.output_dir)
    else:
        generate_category(args.category, args.data_dir, args.output_dir)

    logger.info(f"\n{'='*50}")
    logger.info("✓ Complete!")
    logger.info(f"{'='*50}")

if __name__ == '__main__':
    main()
