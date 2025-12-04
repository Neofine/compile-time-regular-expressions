"""Figure generation package."""

from .base import BaseFigure, save_figure
from .line_plots import TimeSeriesPlot, SpeedupPlot, MultiPatternPlot
from .bar_plots import BarComparison, ScalingBars
from .heatmaps import SpeedupHeatmap, WorstCaseHeatmap
from .compile_plots import CompileOverheadPlot, SIMDOverheadPlot

__all__ = [
    'BaseFigure',
    'save_figure',
    'TimeSeriesPlot',
    'SpeedupPlot',
    'MultiPatternPlot',
    'BarComparison',
    'ScalingBars',
    'SpeedupHeatmap',
    'WorstCaseHeatmap',
    'CompileOverheadPlot',
    'SIMDOverheadPlot',
]
