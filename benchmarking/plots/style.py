"""Unified styling configuration for thesis plots."""
import math

import matplotlib.colors as mcolors
import matplotlib.pyplot as plt
import pandas as pd
import seaborn as sns

FIGURE_SIZES = {
    'single': (8, 5),
    'wide': (12, 5),
    'double': (14, 5),
    'heatmap_small': (8, 4),
    'heatmap_large': (12, 6),
    'bar': (10, 6),
}

ENGINE_COLORS = {
    'CTRE': '#d1e5f0',
    'CTRE-SIMD': '#2166ac',
    'CTRE-Scalar': '#67a9cf',
    'Hyperscan': '#f4a582',
    'RE2': '#d6604d',
    'PCRE2': '#8073ac',
    'std::regex': '#666666',
}

CATEGORY_COLORS = {
    'Simple': '#66c2a5',
    'Complex': '#fc8d62',
    'Behemoth': '#8da0cb',  # Maps to "ExtraLarge" in display
    'ExtraLarge': '#8da0cb',
    'Scaling': '#e78ac3',
    'RealWorld': '#a6d854',
    'NonMatch': '#ffd92f',
    'Ineligible': '#b3b3b3',
}

ACCENT = {
    'baseline_line': '#333333',
    'grid': '#cccccc',
    'annotation': '#444444',
}

ENGINE_ORDER = ['CTRE-SIMD', 'CTRE-Scalar', 'CTRE', 'Hyperscan', 'RE2', 'PCRE2', 'std::regex']
ENGINE_ORDER_HEATMAP = ['std::regex', 'RE2', 'PCRE2', 'Hyperscan', 'CTRE', 'CTRE-Scalar', 'CTRE-SIMD']

ENGINE_STYLES = {
    engine: {
        'color': ENGINE_COLORS[engine],
        'marker': 'o',
        'linestyle': '-',
        'linewidth': 2.0 if engine == 'CTRE-SIMD' else 1.5,
        'markersize': 6 if engine == 'CTRE-SIMD' else 5,
        'zorder': 10 if engine == 'CTRE-SIMD' else 5 - i,
    }
    for i, engine in enumerate(ENGINE_ORDER)
}

ENGINE_LABELS = {e: e for e in ENGINE_ORDER}

PATTERN_REGEX = {
    'digits': '[0-9]+', 'lowercase': '[a-z]+', 'uppercase': '[A-Z]+',
    'vowels': '[aeiou]+', 'alphanumeric': '[a-zA-Z0-9]+',
    'decimal': r'[0-9]+\.[0-9]+', 'hex': '[0-9a-fA-F]+',
    'identifier': '[a-zA-Z_][a-zA-Z0-9_]*', 'url': 'http://[a-z]+',
    'key_value': '[a-z]+=[0-9]+', 'http_method': '(GET|POST)/[a-z]+',
    'letters_digits': '[a-z]+[0-9]+', 'http_header': 'Header: value',
    'log_time': '[0-9]+:[0-9]+:[0-9]+',
    'alt_2': '(a|b)+', 'alt_4': '(a|b|c|d)+',
    'class_2': '[ab]+', 'class_4': '[abcd]+', 'class_26': '[a-z]+',
    'ipv4': r'N.N.N.N', 'uuid': 'hex-hex-hex-hex-hex',
    'email': '[a-z]+@[a-z]+.[a-z]+', 'date': '[0-9]+-[0-9]+-[0-9]+',
    'digits_on_letters': '[0-9]+ (no match)', 'letters_on_digits': '[a-z]+ (no match)',
    'url_on_digits': 'http://[a-z]+ (no match)',
    'dom_suffix': '[a-z]+test', 'dom_prefix': 'test[a-z]+',
    'dom_middle': '[a-z]+test[0-9]+', 'dom_alt': '(foo|bar)test',
    'region_suffix': '(runn|jump|walk)ing', 'dom_url': 'http://[a-z]+.[a-z]+',
    'backref_repeat': r'(.)\1+', 'lazy_star': '[a-z]*?x', 'lazy_plus': '[a-z]+?x',
    'lookahead_pos': '[a-z](?=[0-9])', 'lookahead_neg': '[a-z](?![0-9])',
    'group_repeat': '(abc)+', 'nested_backref': r'((.)\\2)+',
}

FONT_CONFIG = {
    'family': 'serif',
    'serif': ['Times New Roman', 'DejaVu Serif', 'Palatino'],
    'mathtext': 'stix',
    'sizes': {'title': 13, 'label': 12, 'tick': 10, 'legend': 9, 'annotation': 9, 'footnote': 8},
}

PLOT_PARAMS = {
    'grid_alpha': 0.3, 'grid_linestyle': '--',
    'line_width': 1.8, 'marker_size': 6,
    'bar_width': 0.15, 'bar_edge_color': 'white', 'bar_edge_width': 0.5,
    'heatmap_linewidth': 2, 'heatmap_linecolor': 'white',
    'legend_framealpha': 0.95, 'legend_edgecolor': '0.8',
    'display_dpi': 150, 'save_dpi': 300,
}


def get_heatmap_cmap():
    """Custom diverging colormap: pink (slower) -> beige (same) -> blue (faster)."""
    cdict = {
        'red': [(0.0, 0.91, 0.91), (0.4, 0.91, 0.91), (0.5, 0.94, 0.94),
                (0.6, 0.53, 0.53), (0.75, 0.20, 0.20), (1.0, 0.0, 0.0)],
        'green': [(0.0, 0.35, 0.35), (0.4, 0.35, 0.35), (0.5, 0.90, 0.90),
                  (0.6, 0.81, 0.81), (0.75, 0.60, 0.60), (1.0, 0.30, 0.30)],
        'blue': [(0.0, 0.61, 0.61), (0.4, 0.61, 0.61), (0.5, 0.55, 0.55),
                 (0.6, 0.98, 0.98), (0.75, 0.85, 0.85), (1.0, 0.70, 0.70)]
    }
    return mcolors.LinearSegmentedColormap('SpeedupCmap', cdict)


def get_pattern_label(name: str, max_len: int = 20) -> str:
    """Return regex if short, otherwise the pattern name."""
    regex = PATTERN_REGEX.get(name, name)
    return regex if len(regex) <= max_len else name


def setup_style():
    """Apply consistent styling to all matplotlib plots."""
    sns.set_theme(style="whitegrid", font_scale=1.0)
    plt.rcParams.update({
        'font.family': FONT_CONFIG['family'],
        'font.serif': FONT_CONFIG['serif'],
        'mathtext.fontset': FONT_CONFIG['mathtext'],
        'font.size': FONT_CONFIG['sizes']['label'],
        'axes.labelsize': FONT_CONFIG['sizes']['label'],
        'axes.titlesize': FONT_CONFIG['sizes']['title'],
        'axes.titleweight': 'bold',
        'axes.spines.top': False,
        'axes.spines.right': False,
        'axes.grid': True,
        'grid.alpha': PLOT_PARAMS['grid_alpha'],
        'grid.linestyle': PLOT_PARAMS['grid_linestyle'],
        'xtick.labelsize': FONT_CONFIG['sizes']['tick'],
        'ytick.labelsize': FONT_CONFIG['sizes']['tick'],
        'legend.fontsize': FONT_CONFIG['sizes']['legend'],
        'legend.framealpha': PLOT_PARAMS['legend_framealpha'],
        'legend.edgecolor': PLOT_PARAMS['legend_edgecolor'],
        'lines.linewidth': PLOT_PARAMS['line_width'],
        'lines.markersize': PLOT_PARAMS['marker_size'],
        'figure.dpi': PLOT_PARAMS['display_dpi'],
        'savefig.dpi': PLOT_PARAMS['save_dpi'],
        'savefig.facecolor': 'white',
        'savefig.bbox': 'tight',
    })


def format_size(x):
    """Format input size as power of 2."""
    return f'$2^{{{int(round(math.log2(x)))}}}$' if x > 0 else str(int(x))


def format_time(ns):
    """Format time in appropriate units."""
    if ns >= 1e6:
        return f'{ns/1e6:.1f}ms'
    if ns >= 1e3:
        return f'{ns/1e3:.1f}μs'
    return f'{ns:.0f}ns'


def format_speedup(val):
    """Format speedup value."""
    if pd.isna(val):
        return 'N/A'
    return f'{val:.2f}' if val < 1 else f'{val:.1f}'
