#!/usr/bin/env python3
import argparse
import logging
import shutil
import sys
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

SCRIPT_DIR = Path(__file__).parent.resolve()
sys.path.insert(0, str(SCRIPT_DIR))

from plots import (ENGINE_ORDER, format_size, get_pattern_label, setup_style,
                   load_benchmark_results, load_compile_overhead, load_compile_time,
                   load_csv, merge_simd_baseline, BenchmarkData,
                   BarComparison, CompileOverheadPlot, SIMDOverheadPlot,
                   SpeedupHeatmap, TimeSeriesPlot)

logging.basicConfig(level=logging.INFO, format='%(message)s')
logger = logging.getLogger(__name__)

DATA_DIR = SCRIPT_DIR / 'output'
OUTPUT_DIR = SCRIPT_DIR / 'output' / 'figures'
LEGACY_DATA_DIR = SCRIPT_DIR / 'output' / 'thesis'

CATEGORIES = {
    'simple': {'prefix': 'Simple/', 'title': 'Simple Patterns', 'heatmap_size': 1024},
    'complex': {'prefix': 'Complex/', 'title': 'Complex Patterns', 'heatmap_size': 1024},
    'scaling': {'prefix': 'Scaling/', 'title': 'Pattern Scaling', 'heatmap_size': 1024},
    'realworld': {'prefix': 'RealWorld/', 'title': 'Real-World Workloads', 'heatmap_size': 1024},
    'production': {'prefix': 'Production/', 'title': 'Production Patterns', 'heatmap_size': 1024},
    'nomatch': {'prefix': 'NonMatch/', 'title': 'Non-Matching Inputs', 'heatmap_size': 1024},
    'small': {'prefix': 'Small/', 'title': 'Small Inputs', 'heatmap_size': 16, 'log_x': False},
    'large': {'prefix': 'Large/', 'title': 'Large Inputs', 'heatmap_size': 65536},
    'fallback': {'prefix': 'Fallback/', 'title': 'Fallback Patterns', 'heatmap_size': 1024, 
                 'exclude_patterns': ['lookahead_pos']},  # degenerate case - pattern can never match
    'adversarial': {'prefix': 'Adversarial/', 'title': 'Adversarial Patterns', 'heatmap_size': 16, 'skip_heatmap': True},
    'instantiation': {'prefix': 'Instantiation/', 'title': 'Instantiation Time', 'custom_plot': 'instantiation'},
    'arm': {'prefix': 'arm/', 'title': 'ARM Patterns', 'heatmap_size': 256, 'skip_heatmap': True},
    'arm_nomatch': {'prefix': 'arm_nomatch/', 'title': 'ARM Non-Match', 'heatmap_size': 256, 'skip_heatmap': True},
}

ENGINE_COLORS = {
    'CTRE-SIMD': '#2166ac', 'CTRE-Scalar': '#67a9cf', 'CTRE': '#d1e5f0',
    'RE2': '#d6604d', 'PCRE2': '#8073ac', 'Hyperscan': '#f4a582', 'std::regex': '#666666',
}


def generate_time_series(data, output_dir, config):
    prefix, log_x = config['prefix'], config.get('log_x', True)
    for pattern in [p for p in data.patterns if p.startswith(prefix)]:
        name = pattern.split('/')[-1]
        plot = TimeSeriesPlot(title=get_pattern_label(name), log_x=log_x)
        plot.plot(data, pattern)
        plot.save(output_dir / f'{name}_time.png')


def generate_heatmap(data, output_dir, config, size=1024):
    title = f"{config['title']} — Speedup ({format_size(size)})"
    exclude = config.get('exclude_patterns', [])
    plot = SpeedupHeatmap(title=title)
    plot.plot(data, size=size, pattern_prefix=config['prefix'], exclude_patterns=exclude)
    plot.save(output_dir / 'heatmap.png')


def generate_bar_chart(data, output_dir, config, size=1024):
    prefix = config['prefix']
    patterns = [p for p in data.patterns if p.startswith(prefix)]
    
    actual_sizes, valid_patterns = [], []
    for pattern in patterns:
        available = data.filter_pattern(pattern)['Input_Size'].unique()
        use_size = size if size in available else (available[0] if len(available) > 0 else None)
        if use_size:
            actual_sizes.append(use_size)
            valid_patterns.append(pattern)
    
    if not valid_patterns:
        return
    
    if len(set(actual_sizes)) > 1:
        _generate_variable_bar_chart(data, output_dir, valid_patterns, actual_sizes, config['title'])
    else:
        labels = [get_pattern_label(p.split('/')[-1]) for p in valid_patterns]
        plot = BarComparison(title=f"{config['title']} — Comparison ({format_size(actual_sizes[0])})")
        plot.plot(data, valid_patterns, labels, size=actual_sizes[0])
        plot.save(output_dir / 'bar_comparison.png')


def _generate_variable_bar_chart(data, output_dir, patterns, sizes, title):
    fig, ax = plt.subplots(figsize=(12, 6))
    x, width = np.arange(len(patterns)), 0.12
    engines = ['CTRE-SIMD', 'CTRE-Scalar', 'CTRE', 'RE2', 'PCRE2', 'Hyperscan', 'std::regex']
    
    for i, engine in enumerate(engines):
        times = [data.get_time(p, engine, s) or 0 for p, s in zip(patterns, sizes)]
        if any(t > 0 for t in times):
            ax.bar(x + (i - len(engines)/2 + 0.5) * width, times, width,
                   label=engine, color=ENGINE_COLORS.get(engine, '#333'))
    
    labels = [f"{get_pattern_label(p.split('/')[-1])}\n({format_size(s)})" for p, s in zip(patterns, sizes)]
    ax.set_xlabel('Pattern')
    ax.set_ylabel('Time (ns)')
    ax.set_title(f"{title} — Comparison", fontweight='bold')
    ax.set_xticks(x)
    ax.set_xticklabels(labels, rotation=45, ha='right', fontsize=9)
    ax.set_yscale('log')
    ax.legend(loc='upper right', fontsize=9)
    plt.tight_layout()
    fig.savefig(output_dir / 'bar_comparison.png', dpi=150, bbox_inches='tight', facecolor='white')
    plt.close(fig)


def generate_speedup_bars(data, output_dir, config, size=1024):
    prefix = config['prefix']
    patterns = [p for p in data.patterns if p.startswith(prefix)]
    
    speedups, actual_sizes = [], []
    for pattern in patterns:
        available = data.filter_pattern(pattern)['Input_Size'].unique()
        use_size = size if size in available else (available[0] if len(available) > 0 else size)
        actual_sizes.append(use_size)
        
        scalar = data.get_time(pattern, 'CTRE-Scalar', use_size) or data.get_time(pattern, 'CTRE-SIMD', use_size)
        base = data.get_time(pattern, 'CTRE', use_size)
        speedups.append(base / scalar if scalar and base and scalar > 0 else 1.0)

    fig, ax = plt.subplots(figsize=(10, 6))
    x = np.arange(len(patterns))
    colors = ['#2166ac' if s >= 1.0 else '#c51b7d' for s in speedups]
    bars = ax.bar(x, speedups, color=colors, edgecolor='white', linewidth=0.5)
    ax.axhline(y=1.0, color='#666', linestyle='--', linewidth=1, alpha=0.7)

    labels = [get_pattern_label(p.split('/')[-1]) for p in patterns]
    if len(set(actual_sizes)) > 1:
        labels = [f"{l}\n({format_size(s)})" for l, s in zip(labels, actual_sizes)]

    for bar, s in zip(bars, speedups):
        ax.annotate(f'{s:.2f}x', xy=(bar.get_x() + bar.get_width()/2, bar.get_height()),
                    xytext=(0, 3), textcoords="offset points", ha='center', va='bottom', fontsize=8)

    ax.set_xlabel('Pattern')
    ax.set_ylabel('Speedup')
    ax.set_title(f"{config['title']} — Speedup", fontweight='bold')
    ax.set_xticks(x)
    ax.set_xticklabels(labels, rotation=45, ha='right', fontsize=9)
    ax.set_ylim(bottom=0)
    plt.tight_layout()
    fig.savefig(output_dir / 'speedup_bars.png', dpi=150, bbox_inches='tight', facecolor='white')
    plt.close(fig)


def generate_instantiation_plot(df, output_dir, config):
    prefix = config['prefix']
    patterns = sorted([p for p in df['Pattern'].unique() if p.startswith(prefix)])
    if not patterns:
        return

    engines = ['std::regex', 'PCRE2', 'RE2', 'Hyperscan', 'CTRE-SIMD']
    fig, ax = plt.subplots(figsize=(12, 6))
    x, width = np.arange(len(patterns)), 0.15

    for i, engine in enumerate(engines):
        times = []
        for pattern in patterns:
            pat_data = df[(df['Pattern'] == pattern) & (df['Engine'] == engine)]
            times.append(pat_data['Time_ns'].values[0] if len(pat_data) > 0 else 0)
        
        bars = ax.bar(x + (i - len(engines)/2 + 0.5) * width, [max(t, 0.1) for t in times],
                      width, label=engine, color=ENGINE_COLORS.get(engine, '#999'))
        
        for bar, t in zip(bars, times):
            label = '0ns' if t == 0 else (f'{t/1e6:.1f}ms' if t >= 1e6 else 
                                          f'{t/1e3:.0f}μs' if t >= 1e3 else f'{t:.0f}ns')
            ax.annotate(label, xy=(bar.get_x() + bar.get_width()/2, bar.get_height()),
                        xytext=(0, 3), textcoords='offset points', ha='center', fontsize=6, rotation=90)

    ax.set_xlabel('Pattern')
    ax.set_ylabel('Time (ns)')
    ax.set_title('Regex Instantiation Time', fontweight='bold')
    ax.set_xticks(x)
    ax.set_xticklabels([p.split('/')[-1] for p in patterns], rotation=30, ha='right')
    ax.set_yscale('log')
    ax.legend(loc='upper left')
    plt.tight_layout()
    plt.savefig(output_dir / 'instantiation_time.png', dpi=150, bbox_inches='tight')
    plt.close()


def generate_category(category: str, data_dir: Path, output_dir: Path) -> bool:
    config = CATEGORIES.get(category)
    if not config:
        logger.error(f"Unknown category: {category}")
        return False

    logger.info(f"\n{'='*50}\n  {config['title']}\n{'='*50}")

    data = load_benchmark_results(data_dir, category) or load_benchmark_results(LEGACY_DATA_DIR, category)
    if data is None:
        logger.warning(f"  No data for {category}")
        return False

    patterns = [p for p in data.patterns if p.startswith(config['prefix'])]
    logger.info(f"  {len(patterns)} patterns, {len(data.engines)} engines")

    if not patterns:
        logger.warning(f"  No patterns with prefix '{config['prefix']}'")
        return False

    cat_output = output_dir / category
    cat_output.mkdir(parents=True, exist_ok=True)

    if config.get('custom_plot') == 'instantiation':
        try:
            generate_instantiation_plot(data.df, cat_output, config)
            logger.info("  ✓ Instantiation plot")
        except Exception as e:
            logger.error(f"  ✗ Instantiation: {e}")
        return True

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
    logger.info(f"\n{'='*50}\n  Compilation Overhead\n{'='*50}")
    df = load_compile_overhead(data_dir) or load_compile_overhead(LEGACY_DATA_DIR)
    if df is None:
        logger.warning("  No compile data")
        return

    cat_output = output_dir / 'compile_overhead'
    cat_output.mkdir(parents=True, exist_ok=True)

    try:
        plot = CompileOverheadPlot(title='Compilation Overhead')
        plot.plot(df)
        plot.save(cat_output / 'compile_summary.png')
        logger.info("  ✓ Compile summary")
    except Exception as e:
        logger.error(f"  ✗ {e}")

    try:
        plot = SIMDOverheadPlot(title='SIMD Overhead')
        plot.plot(df)
        plot.save(cat_output / 'simd_overhead.png')
        logger.info("  ✓ SIMD overhead")
    except Exception as e:
        logger.error(f"  ✗ {e}")


def generate_codesize(data_dir: Path, output_dir: Path):
    logger.info(f"\n{'='*50}\n  Code Size\n{'='*50}")
    csv_path = data_dir / 'codesize.csv'
    if not csv_path.exists():
        logger.warning("  No codesize data")
        return

    df = pd.read_csv(csv_path)
    cat_output = output_dir / 'codesize'
    cat_output.mkdir(parents=True, exist_ok=True)
    size_col = 'Text_Bytes' if 'Text_Bytes' in df.columns else 'Binary_Size_KB'

    try:
        setup_style()
        fig, ax = plt.subplots(figsize=(10, 6))
        patterns = df['Pattern'].unique()
        x, width = np.arange(len(patterns)), 0.35

        baseline = df[df['Variant'] == 'Baseline'][size_col].values / 1024
        simd = df[df['Variant'] == 'SIMD'][size_col].values / 1024

        ax.bar(x - width/2, baseline, width, label='Baseline', color='#92c5de')
        ax.bar(x + width/2, simd, width, label='SIMD', color='#2166ac')

        ax.set_xlabel('Pattern')
        ax.set_ylabel('Code Size (KB)')
        ax.set_title('Code Size: Baseline vs SIMD')
        ax.set_xticks(x)
        ax.set_xticklabels([get_pattern_label(p) for p in patterns], rotation=30, ha='right')
        ax.legend()
        plt.tight_layout()
        fig.savefig(cat_output / 'codesize_comparison.png', dpi=300, bbox_inches='tight')
        plt.close(fig)
        logger.info("  ✓ Codesize comparison")
    except Exception as e:
        logger.error(f"  ✗ {e}")


def generate_compile_time(data_dir: Path, output_dir: Path):
    logger.info(f"\n{'='*50}\n  Compilation Time\n{'='*50}")
    df = load_compile_time(data_dir)
    if df is None:
        logger.warning("  No compile time data")
        return

    cat_output = output_dir / 'compile_time'
    cat_output.mkdir(parents=True, exist_ok=True)

    try:
        setup_style()
        categories = df['Category'].unique()
        fig, ax = plt.subplots(figsize=(12, 6))
        x, width = np.arange(len(categories)), 0.35

        baseline = [df[(df['Category'] == c) & (df['Variant'] == 'Baseline')]['Compile_Time_s'].mean() for c in categories]
        simd = [df[(df['Category'] == c) & (df['Variant'] == 'SIMD')]['Compile_Time_s'].mean() for c in categories]

        ax.bar(x - width/2, baseline, width, label='Baseline', color='#92c5de')
        ax.bar(x + width/2, simd, width, label='SIMD', color='#2166ac')

        ax.set_xlabel('Category')
        ax.set_ylabel('Compile Time (s)')
        ax.set_title('Compilation Time')
        ax.set_xticks(x)
        ax.set_xticklabels(categories)
        ax.legend()
        plt.tight_layout()
        fig.savefig(cat_output / 'compile_time_by_category.png', dpi=300, bbox_inches='tight')
        plt.close(fig)
        logger.info("  ✓ Compile time")
    except Exception as e:
        logger.error(f"  ✗ {e}")


def main():
    parser = argparse.ArgumentParser(description='Generate benchmark plots')
    parser.add_argument('--category', '-c', choices=list(CATEGORIES.keys()) + ['all', 'compile', 'codesize', 'compiletime'], default='all')
    parser.add_argument('--data-dir', '-d', type=Path, default=DATA_DIR)
    parser.add_argument('--output-dir', '-o', type=Path, default=OUTPUT_DIR)
    parser.add_argument('--list', '-l', action='store_true', help='List categories')
    parser.add_argument('--clean', action='store_true', help='Clean output before generating')
    args = parser.parse_args()

    if args.list:
        for cat, cfg in CATEGORIES.items():
            print(f"  {cat}: {cfg['title']}")
        return

    setup_style()
    logger.info(f"Data: {args.data_dir}\nOutput: {args.output_dir}")

    if args.clean and args.output_dir.exists():
        shutil.rmtree(args.output_dir)
    args.output_dir.mkdir(parents=True, exist_ok=True)

    if args.category == 'all':
        for cat in CATEGORIES:
            generate_category(cat, args.data_dir, args.output_dir)
        generate_compile_overhead(args.data_dir, args.output_dir)
        generate_codesize(args.data_dir, args.output_dir)
        generate_compile_time(args.data_dir, args.output_dir)
    elif args.category == 'compile':
        generate_compile_overhead(args.data_dir, args.output_dir)
    elif args.category == 'codesize':
        generate_codesize(args.data_dir, args.output_dir)
    elif args.category == 'compiletime':
        generate_compile_time(args.data_dir, args.output_dir)
    else:
        generate_category(args.category, args.data_dir, args.output_dir)

    logger.info(f"\n{'='*50}\n✓ Complete!\n{'='*50}")


if __name__ == '__main__':
    main()
