"""
Bar chart figures for comparison visualizations.
"""

import matplotlib.pyplot as plt
import numpy as np
from pathlib import Path
from typing import List, Optional, Dict

from .base import BaseFigure
from ..config import (
    ENGINE_ORDER, ENGINE_STYLES, ENGINE_LABELS,
    FIGURE_SIZES, PLOT_PARAMS, format_size
)
from ..data import BenchmarkData

# ============================================================================
# BAR COMPARISON
# ============================================================================

class BarComparison(BaseFigure):
    """
    Grouped bar chart comparing engines across patterns.
    Shows absolute times or speedups.
    """

    def __init__(
        self,
        title: str = None,
        ylabel: str = 'Matching Time (ns)',
        log_scale: bool = True,
        show_speedup: bool = False,
        baseline_engine: str = 'CTRE',
    ):
        super().__init__(figsize=FIGURE_SIZES['wide'], title=title)
        self.ylabel = ylabel
        self.log_scale = log_scale
        self.show_speedup = show_speedup
        self.baseline_engine = baseline_engine

    def plot(
        self,
        data: BenchmarkData,
        patterns: List[str],
        labels: List[str] = None,
        engines: List[str] = None,
        size: int = 1024,
    ):
        """
        Create grouped bar chart.

        Args:
            data: Benchmark data
            patterns: Patterns to include
            labels: Display labels for patterns
            engines: Engines to compare
            size: Input size to use
        """
        self.create_figure()

        labels = labels or [p.split('/')[-1] for p in patterns]
        engines = engines or [e for e in ENGINE_ORDER if e != self.baseline_engine]

        size_data = data.filter_size(size)
        x = np.arange(len(patterns))
        width = PLOT_PARAMS['bar_width']
        n_engines = len(engines)

        # Get baseline times for speedup calculation
        baseline_times = {}
        if self.show_speedup:
            for pat in patterns:
                pat_data = size_data[
                    (size_data['Pattern'] == pat) &
                    (size_data['Engine'] == self.baseline_engine)
                ]
                if not pat_data.empty:
                    baseline_times[pat] = pat_data['Time_ns'].values[0]

        for i, engine in enumerate(engines):
            values = []
            for pat in patterns:
                pat_data = size_data[
                    (size_data['Pattern'] == pat) &
                    (size_data['Engine'] == engine)
                ]

                if pat_data.empty:
                    values.append(0)
                elif self.show_speedup and pat in baseline_times:
                    values.append(baseline_times[pat] / pat_data['Time_ns'].values[0])
                else:
                    values.append(pat_data['Time_ns'].values[0])

            offset = (i - n_engines/2 + 0.5) * width
            style = ENGINE_STYLES.get(engine, ENGINE_STYLES['CTRE'])

            bars = self.ax.bar(
                x + offset,
                values,
                width,
                label=ENGINE_LABELS.get(engine, engine),
                color=style['color'],
                edgecolor=PLOT_PARAMS['bar_edge_color'],
                linewidth=PLOT_PARAMS['bar_edge_width'],
                zorder=3,
            )

            # Add value annotations for primary engine
            if engine == 'CTRE-SIMD':
                for bar in bars:
                    height = bar.get_height()
                    if height > 0:
                        label_text = f'{height:.1f}×' if self.show_speedup else f'{height:.0f}'
                        self.ax.annotate(
                            label_text,
                            xy=(bar.get_x() + bar.get_width()/2, height),
                            xytext=(0, 2),
                            textcoords='offset points',
                            ha='center',
                            va='bottom',
                            fontsize=8,
                            color=style['color'],
                            fontweight='bold',
                        )

        # Reference line for speedup
        if self.show_speedup:
            self.ax.axhline(y=1.0, color='#333333', linestyle='-', linewidth=1, zorder=1)
            self.ax.text(len(patterns)-0.5, 1.05, 'baseline', fontsize=8, color='#333333', ha='right')

        # Formatting
        self.ax.set_xticks(x)
        self.ax.set_xticklabels(labels, rotation=30, ha='right')
        self.ax.set_ylabel(self.ylabel)

        if self.log_scale:
            self.ax.set_yscale('log')

        self.ax.legend(loc='upper right', fontsize=8)
        self.ax.set_axisbelow(True)

        return self

# ============================================================================
# SCALING BARS
# ============================================================================

class ScalingBars(BaseFigure):
    """
    Bar chart showing how performance scales with pattern complexity.
    Used for alternation, repetition, and class size scaling.
    """

    def __init__(
        self,
        title: str = None,
        xlabel: str = 'Pattern Complexity',
        ylabel: str = 'Matching Time (ns)',
        log_scale: bool = True,
    ):
        super().__init__(figsize=FIGURE_SIZES['bar'], title=title)
        self.xlabel = xlabel
        self.ylabel = ylabel
        self.log_scale = log_scale

    def plot(
        self,
        data: BenchmarkData,
        patterns: List[str],
        labels: List[str],
        engines: List[str] = None,
        size: int = 1024,
    ):
        """
        Create scaling comparison bar chart.

        Args:
            data: Benchmark data
            patterns: Patterns in scaling order
            labels: X-axis labels for each pattern
            engines: Engines to compare
            size: Input size to use
        """
        self.create_figure()

        engines = engines or ['CTRE-SIMD', 'CTRE', 'RE2', 'PCRE2', 'Hyperscan']
        size_data = data.filter_size(size)

        x = np.arange(len(patterns))
        width = PLOT_PARAMS['bar_width']
        n_engines = len(engines)

        for i, engine in enumerate(engines):
            times = []
            for pat in patterns:
                pat_data = size_data[
                    (size_data['Pattern'] == pat) &
                    (size_data['Engine'] == engine)
                ]
                times.append(pat_data['Time_ns'].values[0] if not pat_data.empty else 0)

            offset = (i - n_engines/2 + 0.5) * width
            style = ENGINE_STYLES.get(engine, ENGINE_STYLES['CTRE'])

            self.ax.bar(
                x + offset,
                times,
                width,
                label=ENGINE_LABELS.get(engine, engine),
                color=style['color'],
                edgecolor=PLOT_PARAMS['bar_edge_color'],
                linewidth=PLOT_PARAMS['bar_edge_width'],
            )

        self.ax.set_xlabel(self.xlabel)
        self.ax.set_ylabel(self.ylabel)
        self.ax.set_xticks(x)
        self.ax.set_xticklabels(labels)
        self.ax.legend(loc='upper left', fontsize=9)

        if self.log_scale:
            self.ax.set_yscale('log')

        return self

