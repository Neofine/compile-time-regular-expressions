"""
Thesis Plotting Library

A unified plotting infrastructure for regex benchmark visualization.

Usage:
    from plotting import generate_all_plots
    generate_all_plots('/path/to/output')

Or use individual components:
    from plotting.config import setup_style, ENGINE_COLORS
    from plotting.data import load_benchmark_results
    from plotting.figures import TimeSeriesPlot, SpeedupHeatmap
"""

from .config import setup_style
from .data import load_benchmark_results, load_compile_overhead, BenchmarkData
from .figures import (
    TimeSeriesPlot,
    SpeedupPlot,
    BarComparison,
    ScalingBars,
    SpeedupHeatmap,
    CompileOverheadPlot,
)

__version__ = '1.0.0'

__all__ = [
    'setup_style',
    'load_benchmark_results',
    'load_compile_overhead',
    'BenchmarkData',
    'TimeSeriesPlot',
    'SpeedupPlot',
    'BarComparison',
    'ScalingBars',
    'SpeedupHeatmap',
    'CompileOverheadPlot',
]

