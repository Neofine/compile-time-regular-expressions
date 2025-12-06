"""
Line plot figures for time series and scaling visualization.
"""

import matplotlib.pyplot as plt
import numpy as np
from pathlib import Path
from typing import List, Optional

from .base import BaseFigure
from ..config import (
    ENGINE_ORDER, ENGINE_STYLES, ENGINE_LABELS,
    FIGURE_SIZES, format_size
)
from ..data import BenchmarkData

# ============================================================================
# TIME SERIES PLOT
# ============================================================================

class TimeSeriesPlot(BaseFigure):
    """
    Line plot showing time vs input size.
    Used for visualizing how matching time scales with input.
    """

    def __init__(
        self,
        title: str = None,
        xlabel: str = 'Input Size',
        ylabel: str = 'Matching Time (ns)',
        log_x: bool = True,
        log_y: bool = True,
    ):
        super().__init__(figsize=FIGURE_SIZES['single'], title=title)
        self.xlabel = xlabel
        self.ylabel = ylabel
        self.log_x = log_x
        self.log_y = log_y

    def plot(
        self,
        data: BenchmarkData,
        pattern: str,
        engines: List[str] = None,
    ):
        """
        Plot time series for a specific pattern.

        Args:
            data: Benchmark data
            pattern: Pattern to plot (full name like 'Simple/digits')
            engines: List of engines to include (default: all)
        """
        self.create_figure()

        engines = engines or ENGINE_ORDER
        pattern_data = data.filter_pattern(pattern)
        sizes = sorted(pattern_data['Input_Size'].unique())

        for engine in engines:
            eng_data = pattern_data[pattern_data['Engine'] == engine]
            eng_data = eng_data.sort_values('Input_Size')

            if eng_data.empty:
                continue

            style = ENGINE_STYLES.get(engine, ENGINE_STYLES['CTRE'])

            self.ax.plot(
                eng_data['Input_Size'],
                eng_data['Time_ns'],
                marker=style['marker'],
                linestyle=style['linestyle'],
                color=style['color'],
                linewidth=style['linewidth'],
                markersize=style['markersize'],
                label=ENGINE_LABELS.get(engine, engine),
                zorder=style['zorder'],
            )

        # Axis formatting
        self.ax.set_xlabel(self.xlabel)
        self.ax.set_ylabel(self.ylabel)

        if self.log_x:
            self.ax.set_xscale('log', base=2)
            self.ax.set_xticks(sizes)
            self.ax.set_xticklabels([format_size(s) for s in sizes])

        if self.log_y:
            self.ax.set_yscale('log')

        self.ax.legend(loc='upper left', frameon=True)

        return self

# ============================================================================
# SPEEDUP PLOT
# ============================================================================

class SpeedupPlot(BaseFigure):
    """
    Line plot showing speedup ratio vs input size.
    Useful for showing relative performance improvement.
    """

    def __init__(
        self,
        title: str = None,
        baseline_engine: str = 'CTRE',
        xlabel: str = 'Input Size',
        ylabel: str = 'Speedup vs Baseline',
    ):
        super().__init__(figsize=FIGURE_SIZES['single'], title=title)
        self.baseline_engine = baseline_engine
        self.xlabel = xlabel
        self.ylabel = ylabel

    def plot(
        self,
        data: BenchmarkData,
        pattern: str,
        engines: List[str] = None,
    ):
        """
        Plot speedup ratios for a pattern.

        Args:
            data: Benchmark data
            pattern: Pattern to analyze
            engines: Engines to compare (excluding baseline)
        """
        self.create_figure()

        engines = engines or [e for e in ENGINE_ORDER if e != self.baseline_engine]
        pattern_data = data.filter_pattern(pattern)
        sizes = sorted(pattern_data['Input_Size'].unique())

        # Get baseline times
        baseline_data = pattern_data[pattern_data['Engine'] == self.baseline_engine]
        baseline_times = dict(zip(baseline_data['Input_Size'], baseline_data['Time_ns']))

        for engine in engines:
            eng_data = pattern_data[pattern_data['Engine'] == engine]

            if eng_data.empty:
                continue

            speedups = []
            valid_sizes = []

            for _, row in eng_data.iterrows():
                size = row['Input_Size']
                if size in baseline_times and baseline_times[size] > 0:
                    speedups.append(baseline_times[size] / row['Time_ns'])
                    valid_sizes.append(size)

            if not speedups:
                continue

            style = ENGINE_STYLES.get(engine, ENGINE_STYLES['CTRE'])

            self.ax.plot(
                valid_sizes,
                speedups,
                marker=style['marker'],
                linestyle=style['linestyle'],
                color=style['color'],
                linewidth=style['linewidth'],
                markersize=style['markersize'],
                label=ENGINE_LABELS.get(engine, engine),
                zorder=style['zorder'],
            )

        # Reference line at 1.0
        self.ax.axhline(y=1.0, color='#333333', linestyle='--', linewidth=1, alpha=0.7)

        # Axis formatting
        self.ax.set_xlabel(self.xlabel)
        self.ax.set_ylabel(self.ylabel)
        self.ax.set_xscale('log', base=2)
        self.ax.set_xticks(sizes)
        self.ax.set_xticklabels([format_size(s) for s in sizes])

        self.ax.legend(loc='best', frameon=True)

        return self

# ============================================================================
# MULTI-PATTERN COMPARISON
# ============================================================================

class MultiPatternPlot(BaseFigure):
    """
    Compare multiple patterns on a single plot.
    """

    def __init__(
        self,
        title: str = None,
        engine: str = 'CTRE-SIMD',
    ):
        super().__init__(figsize=FIGURE_SIZES['single'], title=title)
        self.engine = engine

    def plot(
        self,
        data: BenchmarkData,
        patterns: List[str],
        labels: List[str] = None,
    ):
        """
        Plot multiple patterns for a single engine.

        Args:
            data: Benchmark data
            patterns: List of patterns to compare
            labels: Display labels for patterns
        """
        self.create_figure()

        labels = labels or [p.split('/')[-1] for p in patterns]
        colors = plt.cm.viridis(np.linspace(0.2, 0.8, len(patterns)))

        for i, (pattern, label) in enumerate(zip(patterns, labels)):
            pattern_data = data.filter_pattern(pattern)
            eng_data = pattern_data[pattern_data['Engine'] == self.engine]
            eng_data = eng_data.sort_values('Input_Size')

            if eng_data.empty:
                continue

            self.ax.plot(
                eng_data['Input_Size'],
                eng_data['Time_ns'],
                marker='o',
                color=colors[i],
                label=label,
                linewidth=1.5,
            )

        sizes = sorted(data.sizes)
        self.ax.set_xlabel('Input Size')
        self.ax.set_ylabel('Matching Time (ns)')
        self.ax.set_xscale('log', base=2)
        self.ax.set_yscale('log')
        self.ax.set_xticks(sizes)
        self.ax.set_xticklabels([format_size(s) for s in sizes], fontsize=8)
        self.ax.legend(title='Pattern', fontsize=9)

        return self

