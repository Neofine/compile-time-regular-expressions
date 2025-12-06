"""
Unified styling configuration for all thesis plots.
Single source of truth for colors, fonts, and visual parameters.
"""

import matplotlib.pyplot as plt
import matplotlib.colors as mcolors
import seaborn as sns

# ============================================================================
# FIGURE DIMENSIONS
# ============================================================================

FIGURE_SIZES = {
    'single': (8, 5),        # Single panel figure
    'wide': (12, 5),         # Wide single panel
    'double': (14, 5),       # Two panels side by side
    'heatmap_small': (8, 4), # Small heatmap
    'heatmap_large': (12, 6),# Large heatmap
    'bar': (10, 6),          # Bar chart
}

# ============================================================================
# COLOR PALETTE
# ============================================================================

# Primary engine colors - colorblind-friendly
ENGINE_COLORS = {
    'CTRE-SIMD':  '#2166ac',  # Strong blue - our main result
    'CTRE':       '#92c5de',  # Light blue - baseline
    'Hyperscan':  '#f4a582',  # Orange
    'RE2':        '#d6604d',  # Red-orange
    'PCRE2':      '#8073ac',  # Purple
    'std::regex': '#666666',  # Gray
}

# Category colors for grouping
CATEGORY_COLORS = {
    'Simple': '#66c2a5',     # Teal
    'Complex': '#fc8d62',    # Orange
    'Behemoth': '#8da0cb',   # Blue-purple
    'Scaling': '#e78ac3',    # Pink
    'RealWorld': '#a6d854',  # Green
    'NonMatch': '#ffd92f',   # Yellow
    'Ineligible': '#b3b3b3', # Gray
}

# Heatmap diverging colormap (pink -> beige -> blue)
def get_heatmap_cmap():
    """Custom diverging colormap for speedup heatmaps."""
    cdict = {
        'red':   [(0.0, 0.91, 0.91),   # Pink for slowdowns
                  (0.4, 0.91, 0.91),
                  (0.5, 0.94, 0.94),   # Beige at center
                  (0.6, 0.53, 0.53),   # Light blue
                  (0.75, 0.20, 0.20),  # Medium blue
                  (1.0, 0.0, 0.0)],    # Dark blue for speedups
        'green': [(0.0, 0.35, 0.35),
                  (0.4, 0.35, 0.35),
                  (0.5, 0.90, 0.90),
                  (0.6, 0.81, 0.81),
                  (0.75, 0.60, 0.60),
                  (1.0, 0.30, 0.30)],
        'blue':  [(0.0, 0.61, 0.61),
                  (0.4, 0.61, 0.61),
                  (0.5, 0.55, 0.55),
                  (0.6, 0.98, 0.98),
                  (0.75, 0.85, 0.85),
                  (1.0, 0.70, 0.70)]
    }
    return mcolors.LinearSegmentedColormap('SpeedupCmap', cdict)

# Accent colors
ACCENT = {
    'baseline_line': '#333333',
    'grid': '#cccccc',
    'annotation': '#444444',
}

# ============================================================================
# ENGINE STYLING
# ============================================================================

ENGINE_ORDER = ['CTRE-SIMD', 'CTRE', 'Hyperscan', 'RE2', 'PCRE2', 'std::regex']
ENGINE_ORDER_HEATMAP = ['std::regex', 'RE2', 'PCRE2', 'Hyperscan', 'CTRE-SIMD']

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

ENGINE_LABELS = {
    'CTRE-SIMD': 'CTRE-SIMD',
    'CTRE': 'CTRE (baseline)',
    'RE2': 'RE2',
    'PCRE2': 'PCRE2',
    'Hyperscan': 'Hyperscan',
    'std::regex': 'std::regex',
}

# ============================================================================
# PATTERN REGEX MAPPING
# ============================================================================

PATTERN_REGEX = {
    # Simple
    'digits': '[0-9]+',
    'lowercase': '[a-z]+',
    'uppercase': '[A-Z]+',
    'vowels': '[aeiou]+',
    'alphanumeric': '[a-zA-Z0-9]+',
    # Complex
    'decimal': r'[0-9]+\.[0-9]+',
    'hex': '[0-9a-fA-F]+',
    'identifier': '[a-zA-Z_][a-zA-Z0-9_]*',
    'url': 'http://[a-z]+',
    'key_value': '[a-z]+=[0-9]+',
    'http_method': '(GET|POST)/[a-z]+',
    'letters_digits': '[a-z]+[0-9]+',
    'http_header': r'[A-Za-z\-]+: ...',
    'log_time': '[0-9]+:[0-9]+:[0-9]+',
    # Scaling
    'alt_2': '(a|b)+',
    'alt_4': '(a|b|c|d)+',
    'class_2': '[ab]+',
    'class_4': '[abcd]+',
    'class_26': '[a-z]+',
    # RealWorld
    'ipv4': r'[0-9]+(\.[0-9]+){3}',
    'uuid': '[0-9a-f]+-...',
    'email': '[a-z]+@[a-z]+.[a-z]+',
    'date': '[0-9]+-[0-9]+-[0-9]+',
    # NonMatch
    'digits_on_letters': '[0-9]+ on letters',
    'letters_on_digits': '[a-z]+ on digits',
    'url_on_digits': 'http://... on digits',
    'dom_suffix': '[a-z]+test',
    'dom_prefix': 'test[a-z]+',
    'dom_middle': '[a-z]+test[0-9]+',
    'dom_alt': '(foo|bar)test',
    'region_suffix': '(runn|jump|walk)ing',
    'dom_url': 'http://[a-z]+.[a-z]+',
    # Fallback (no SIMD)
    'backref_repeat': r'(.)\1+',
    'lazy_star': '[a-z]*?x',
    'lazy_plus': '[a-z]+?x',
    'lookahead_pos': '[a-z](?=[0-9])',
    'lookahead_neg': '[a-z](?![0-9])',
    'group_repeat': '(abc)+',
    'nested_backref': r'((.)\\2)+',
}

def get_pattern_label(name: str, max_len: int = 20) -> str:
    """Get display label for pattern - show regex if short, name otherwise."""
    regex = PATTERN_REGEX.get(name, name)
    if len(regex) <= max_len:
        return regex
    return name

# ============================================================================
# TYPOGRAPHY
# ============================================================================

FONT_CONFIG = {
    'family': 'serif',
    'serif': ['Times New Roman', 'DejaVu Serif', 'Palatino'],
    'mathtext': 'stix',
    'sizes': {
        'title': 13,
        'label': 12,
        'tick': 10,
        'legend': 9,
        'annotation': 9,
        'footnote': 8,
    }
}

# ============================================================================
# PLOT PARAMETERS
# ============================================================================

PLOT_PARAMS = {
    # Grid
    'grid_alpha': 0.3,
    'grid_linestyle': '--',

    # Lines
    'line_width': 1.8,
    'marker_size': 6,

    # Bars
    'bar_width': 0.15,
    'bar_edge_color': 'white',
    'bar_edge_width': 0.5,

    # Heatmap
    'heatmap_linewidth': 2,
    'heatmap_linecolor': 'white',

    # Legend
    'legend_framealpha': 0.95,
    'legend_edgecolor': '0.8',

    # DPI
    'display_dpi': 150,
    'save_dpi': 300,
}

# ============================================================================
# SETUP FUNCTION
# ============================================================================

def setup_style():
    """Apply consistent styling to all matplotlib plots."""
    # Seaborn base
    sns.set_theme(style="whitegrid", font_scale=1.0)

    # Matplotlib params
    plt.rcParams.update({
        # Font
        'font.family': FONT_CONFIG['family'],
        'font.serif': FONT_CONFIG['serif'],
        'mathtext.fontset': FONT_CONFIG['mathtext'],
        'font.size': FONT_CONFIG['sizes']['label'],

        # Axes
        'axes.labelsize': FONT_CONFIG['sizes']['label'],
        'axes.titlesize': FONT_CONFIG['sizes']['title'],
        'axes.titleweight': 'bold',
        'axes.spines.top': False,
        'axes.spines.right': False,
        'axes.grid': True,

        # Grid
        'grid.alpha': PLOT_PARAMS['grid_alpha'],
        'grid.linestyle': PLOT_PARAMS['grid_linestyle'],

        # Ticks
        'xtick.labelsize': FONT_CONFIG['sizes']['tick'],
        'ytick.labelsize': FONT_CONFIG['sizes']['tick'],

        # Legend
        'legend.fontsize': FONT_CONFIG['sizes']['legend'],
        'legend.framealpha': PLOT_PARAMS['legend_framealpha'],
        'legend.edgecolor': PLOT_PARAMS['legend_edgecolor'],

        # Lines
        'lines.linewidth': PLOT_PARAMS['line_width'],
        'lines.markersize': PLOT_PARAMS['marker_size'],

        # Figure
        'figure.dpi': PLOT_PARAMS['display_dpi'],
        'savefig.dpi': PLOT_PARAMS['save_dpi'],
        'savefig.facecolor': 'white',
        'savefig.bbox': 'tight',
    })

def format_size(x):
    """Format input size as power of 2."""
    import math
    if x > 0:
        exp = int(round(math.log2(x)))
        return f'$2^{{{exp}}}$'
    return str(int(x))

def format_time(ns):
    """Format time in appropriate units."""
    if ns >= 1e6:
        return f'{ns/1e6:.1f}ms'
    elif ns >= 1e3:
        return f'{ns/1e3:.1f}μs'
    else:
        return f'{ns:.0f}ns'

def format_speedup(val):
    """Format speedup value with appropriate precision."""
    if pd.isna(val):
        return 'N/A'
    elif val < 1:
        return f'{val:.2f}'
    else:
        return f'{val:.1f}'

# Import pandas for format_speedup
import pandas as pd
