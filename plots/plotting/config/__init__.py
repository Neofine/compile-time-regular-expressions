"""Configuration package for thesis plots."""

from .style import (
    setup_style,
    ENGINE_COLORS,
    ENGINE_ORDER,
    ENGINE_ORDER_HEATMAP,
    ENGINE_STYLES,
    ENGINE_LABELS,
    CATEGORY_COLORS,
    FIGURE_SIZES,
    FONT_CONFIG,
    PLOT_PARAMS,
    PATTERN_REGEX,
    get_heatmap_cmap,
    get_pattern_label,
    format_size,
    format_time,
    format_speedup,
)

__all__ = [
    'setup_style',
    'ENGINE_COLORS',
    'ENGINE_ORDER',
    'ENGINE_ORDER_HEATMAP',
    'ENGINE_STYLES',
    'ENGINE_LABELS',
    'CATEGORY_COLORS',
    'FIGURE_SIZES',
    'FONT_CONFIG',
    'PLOT_PARAMS',
    'PATTERN_REGEX',
    'get_heatmap_cmap',
    'get_pattern_label',
    'format_size',
    'format_time',
    'format_speedup',
]
