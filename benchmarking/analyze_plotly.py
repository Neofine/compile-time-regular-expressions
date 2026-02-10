#!/usr/bin/env python3
"""Same analysis as analyze.py but charts via Plotly — different look (web-style, rounded, soft).

Run: pip install plotly kaleido pandas
     python3 analyze_plotly.py results.csv -o plots_plotly
"""

import argparse
import sys
from pathlib import Path

try:
    import pandas as pd
    import numpy as np
    import plotly.express as px
    import plotly.graph_objects as go
    from plotly.subplots import make_subplots
except ImportError as e:
    print("Install: pip install plotly kaleido pandas numpy")
    sys.exit(1)

ENGINES = ['CTRE-SIMD', 'CTRE-Scalar', 'CTRE', 'RE2', 'PCRE2', 'Hyperscan', 'std::regex']
# Deliberately different palette from matplotlib (teal/orange/violet/emerald) so Plotly output looks distinct
ENGINE_COLORS = {
    'CTRE-SIMD': '#0d9488',   # teal
    'CTRE-Scalar': '#0891b2', # cyan
    'CTRE': '#06b6d4',
    'RE2': '#ea580c',         # orange
    'PCRE2': '#7c3aed',       # violet
    'Hyperscan': '#ca8a04',   # yellow
    'std::regex': '#64748b',
}

# Visually distinct from matplotlib: blue-gray frame, no white plot card
LAYOUT = dict(
    template='plotly',
    font=dict(family='Open Sans, sans-serif', size=13),
    title_font_size=18,
    margin=dict(l=70, r=50, t=70, b=90),
    paper_bgcolor='#e8eef4',
    plot_bgcolor='#dce4ed',
    xaxis=dict(showgrid=True, gridcolor='rgba(255,255,255,0.6)', zeroline=False),
    yaxis=dict(showgrid=True, gridcolor='rgba(255,255,255,0.6)', zeroline=False),
    legend=dict(orientation='h', yanchor='bottom', y=1.02, xanchor='right', x=1, bgcolor='rgba(255,255,255,0.85)', bordercolor='#94a3b8', borderwidth=1),
    hovermode='x unified',
)

def load_csv(path):
    with open(path) as f:
        lines = [l for l in f if not l.startswith('Running:')]
    from io import StringIO
    return pd.read_csv(StringIO(''.join(lines)))

def format_size(size):
    if size >= 1024 * 1024:
        return f"{size / (1024 * 1024):.0f}MB"
    elif size >= 1024:
        return f"{size / 1024:.0f}KB"
    return f"{size}B"

def find_closest_size(requested, available):
    if requested in available:
        return requested
    return min(available, key=lambda x: abs(x - requested))

def get_time(df, pattern, engine, size):
    row = df[(df['Pattern'] == pattern) & (df['Engine'] == engine) & (df['Input_Size'] == size)]
    return row['Time_ns'].values[0] if len(row) > 0 else None

def get_pattern_label(pattern):
    name = pattern.split('/')[-1]
    return name.replace('_', ' ').title()

def parse_size(s):
    s = s.strip().upper()
    if s.endswith('MB'):
        return int(float(s[:-2]) * 1024 * 1024)
    elif s.endswith('KB'):
        return int(float(s[:-2]) * 1024)
    elif s.endswith('B'):
        return int(s[:-1])
    return int(s)

def plot_comparison(df, output_path, size=None):
    patterns = list(df['Pattern'].unique())
    all_sizes = sorted(df['Input_Size'].unique())
    size = size or find_closest_size(1024, all_sizes)
    df_size = df[df['Input_Size'] == size].copy()
    df_size['Pattern_label'] = df_size['Pattern'].apply(get_pattern_label)
    engines = [e for e in ENGINES if e in df_size['Engine'].unique()]

    fig = go.Figure()
    for engine in engines:
        sub = df_size[df_size['Engine'] == engine]
        fig.add_trace(go.Bar(
            name=engine,
            x=sub['Pattern_label'],
            y=sub['Time_ns'],
            marker_color=ENGINE_COLORS.get(engine, '#64748b'),
            marker_line_color='#334155',
            marker_line_width=1.2,
        ))
    fig.update_layout(
        **LAYOUT,
        title=f'Performance Comparison ({format_size(size)} input)',
        barmode='group',
        bargap=0.25,
        bargroupgap=0.02,
        xaxis_title='Pattern',
        yaxis_title='Matching Time (ns)',
        yaxis_type='log',
        xaxis_tickangle=-30,
    )
    fig.write_image(output_path, scale=2)
    print(f"Saved {output_path}")

def plot_speedup(df, output_path, size=None):
    patterns = list(df['Pattern'].unique())
    all_sizes = sorted(df['Input_Size'].unique())
    size = size or find_closest_size(1024, all_sizes)
    engines = [e for e in ['CTRE-Scalar', 'CTRE', 'RE2', 'PCRE2', 'Hyperscan', 'std::regex'] if e in df['Engine'].unique()]

    fig = go.Figure()
    for engine in engines:
        speedups = []
        for p in patterns:
            simd = get_time(df, p, 'CTRE-SIMD', size)
            other = get_time(df, p, engine, size)
            speedups.append(other / simd if simd and other and simd > 0 else None)
        fig.add_trace(go.Bar(
            name=engine,
            x=[get_pattern_label(p) for p in patterns],
            y=speedups,
            marker_color=ENGINE_COLORS.get(engine, '#64748b'),
            marker_line_color='#334155',
            marker_line_width=1.2,
        ))
    fig.add_hline(y=1.0, line_dash='solid', line_color='#64748b', line_width=1.2, opacity=0.9)
    fig.update_layout(
        **LAYOUT,
        title=f'CTRE-SIMD Speedup ({format_size(size)} input)',
        barmode='group',
        bargap=0.25,
        bargroupgap=0.02,
        xaxis_title='Pattern',
        yaxis_title='Speedup vs CTRE-SIMD',
        xaxis_tickangle=-30,
    )
    fig.write_image(output_path, scale=2)
    print(f"Saved {output_path}")

def plot_speedup_bars(df, output_path, size=None):
    patterns = list(df['Pattern'].unique())
    all_sizes = sorted(df['Input_Size'].unique())
    size = size or find_closest_size(1024, all_sizes)
    speedups = []
    for p in patterns:
        simd = get_time(df, p, 'CTRE-SIMD', size)
        baseline = get_time(df, p, 'CTRE', size) or get_time(df, p, 'CTRE-Scalar', size)
        speedups.append(baseline / simd if simd and baseline and simd > 0 else 1.0)

    colors = ['#059669' if s >= 1 else '#dc2626' if s < 0.5 else '#d97706' for s in speedups]
    fig = go.Figure(go.Bar(
        x=[get_pattern_label(p) for p in patterns],
        y=speedups,
        marker_color=colors,
        marker_line_color='#1e293b',
        marker_line_width=1.5,
        text=[f'{s:.2f}×' for s in speedups],
        textposition='outside',
        textfont_size=12,
    ))
    fig.add_hline(y=1.0, line_dash='dash', line_color='#64748b', line_width=1.2, opacity=0.8)
    fig.update_layout(
        **LAYOUT,
        title=f'CTRE-SIMD Speedup vs Baseline ({format_size(size)} input)',
        xaxis_title='Pattern',
        yaxis_title='Speedup',
        showlegend=False,
        xaxis_tickangle=-30,
    )
    fig.write_image(output_path, scale=2)
    print(f"Saved {output_path}")

def plot_heatmap(df, output_path, size=None, baseline=None):
    """Use seaborn heatmap from analyze.py — Plotly heatmaps don’t look good; seaborn does."""
    try:
        from analyze import plot_heatmap as _mpl_heatmap
        _mpl_heatmap(df, output_path, size=size, baseline=baseline)
    except Exception as e:
        print(f"Heatmap failed: {e}")

def plot_scaling(df, output_path, pattern=None):
    pattern = pattern or df['Pattern'].iloc[0]
    pdf = df[df['Pattern'] == pattern]
    engines = [e for e in ENGINES if e in pdf['Engine'].unique()]
    sizes = sorted(pdf['Input_Size'].unique())

    fig = go.Figure()
    for engine in engines:
        edf = pdf[pdf['Engine'] == engine].sort_values('Input_Size')
        fig.add_trace(go.Scatter(
            x=edf['Input_Size'],
            y=edf['Time_ns'],
            name=engine,
            mode='lines+markers',
            line=dict(width=3, color=ENGINE_COLORS.get(engine, '#64748b')),
            marker=dict(size=11, line=dict(width=1.5, color='#1e293b')),
        ))
    layout = dict(LAYOUT)
    layout.update(
        title=f'Performance Scaling: {get_pattern_label(pattern)}',
        xaxis_title='Input Size',
        yaxis_title='Matching Time (ns)',
        xaxis_type='log',
        yaxis_type='log',
        xaxis_tickvals=sizes,
        xaxis_ticktext=[format_size(s) for s in sizes],
        legend=dict(orientation='h', yanchor='bottom', y=1.02, xanchor='left', x=0),
    )
    fig.update_layout(**layout)
    fig.write_image(output_path, scale=2)
    print(f"Saved {output_path}")

def main():
    parser = argparse.ArgumentParser(description='Analyze benchmark results (Plotly charts)')
    parser.add_argument('csv', help='Input CSV')
    parser.add_argument('-s', '--size', default='1KB')
    parser.add_argument('-o', '--output', default='plots_plotly')
    parser.add_argument('--all', action='store_true')
    args = parser.parse_args()

    df = load_csv(args.csv)
    all_sizes = sorted(df['Input_Size'].unique())
    out = Path(args.output)
    out.mkdir(exist_ok=True)

    if args.all:
        for sz in all_sizes:
            suffix = format_size(sz).lower()
            plot_comparison(df, out / f'comparison_{suffix}.png', sz)
            plot_speedup(df, out / f'speedup_{suffix}.png', sz)
            plot_speedup_bars(df, out / f'speedup_bars_{suffix}.png', sz)
            plot_heatmap(df, out / f'heatmap_{suffix}.png', sz)
        plot_scaling(df, out / 'scaling_all_sizes.png')
    else:
        requested = parse_size(args.size)
        size = find_closest_size(requested, all_sizes)
        suffix = format_size(size).lower()
        plot_comparison(df, out / f'comparison_{suffix}.png', size)
        plot_speedup(df, out / f'speedup_{suffix}.png', size)
        plot_speedup_bars(df, out / f'speedup_bars_{suffix}.png', size)
        plot_heatmap(df, out / f'heatmap_{suffix}.png', size)
        plot_scaling(df, out / 'scaling_all_sizes.png')

if __name__ == '__main__':
    main()
