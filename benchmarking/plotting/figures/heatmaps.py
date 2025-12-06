"""
Heatmap figures for speedup visualization.
"""

import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np
import pandas as pd
from pathlib import Path
from typing import List, Optional

from .base import BaseFigure
from ..config import (
    ENGINE_ORDER_HEATMAP, FIGURE_SIZES, PLOT_PARAMS,
    get_heatmap_cmap, format_speedup, get_pattern_label
)
from ..data import BenchmarkData

# ============================================================================
# SPEEDUP HEATMAP
# ============================================================================

class SpeedupHeatmap(BaseFigure):
    """
    Heatmap showing speedup ratios for engines vs patterns.
    Color-coded: pink (slower), beige (same), blue (faster).
    """

    def __init__(
        self,
        title: str = None,
        baseline_engine: str = 'CTRE',
        vmin: float = 0.1,
        vmax: float = 10.0,
    ):
        super().__init__(figsize=FIGURE_SIZES['heatmap_large'], title=title)
        self.baseline_engine = baseline_engine
        self.vmin = vmin
        self.vmax = vmax

    def plot(
        self,
        data: BenchmarkData,
        patterns: List[str] = None,
        engines: List[str] = None,
        size: int = 1024,
        pattern_prefix: str = None,
    ):
        """
        Create speedup heatmap.

        Args:
            data: Benchmark data
            patterns: Patterns to include (default: all)
            engines: Engines to compare (default: engines in data, excluding baseline)
            size: Input size to use
            pattern_prefix: Filter patterns by prefix (e.g., 'Simple/')
        """
        self.create_figure()

        # Use engines from data, excluding baseline (which would always be 1.0)
        available_engines = [e for e in data.engines if e != self.baseline_engine]
        if engines:
            engines = [e for e in engines if e in available_engines]
        else:
            # Prefer standard order but filter to available
            engines = [e for e in ENGINE_ORDER_HEATMAP if e in available_engines]
            # Add any remaining engines not in standard order
            for e in available_engines:
                if e not in engines:
                    engines.append(e)

        if not engines:
            print(f"Warning: No engines to compare (available: {data.engines}, baseline: {self.baseline_engine})")
            return self

        size_data = data.filter_size(size)

        # Filter patterns
        all_patterns = sorted(data.patterns)
        if pattern_prefix:
            all_patterns = [p for p in all_patterns if p.startswith(pattern_prefix)]
        patterns = patterns or all_patterns

        # Build speedup matrix
        speedup_data = {}
        valid_patterns = []

        for pat in patterns:
            pat_data = size_data[size_data['Pattern'] == pat]
            baseline = pat_data[pat_data['Engine'] == self.baseline_engine]['Time_ns'].values

            if len(baseline) == 0 or baseline[0] <= 0:
                continue

            baseline_time = baseline[0]
            pat_name = pat.split('/')[-1]
            pat_label = get_pattern_label(pat_name)  # Show regex if short
            valid_patterns.append(pat_label)

            for eng in engines:
                if eng not in speedup_data:
                    speedup_data[eng] = {}

                eng_time = pat_data[pat_data['Engine'] == eng]['Time_ns'].values
                if len(eng_time) > 0 and eng_time[0] > 0:
                    speedup_data[eng][pat_label] = baseline_time / eng_time[0]
                else:
                    speedup_data[eng][pat_label] = np.nan

        if not valid_patterns:
            return self

        # Create DataFrame
        heatmap_df = pd.DataFrame(speedup_data).T
        heatmap_df = heatmap_df.reindex(engines)
        heatmap_df = heatmap_df[valid_patterns]

        # Create annotation matrix
        annot = heatmap_df.copy().astype(object)
        for col in heatmap_df.columns:
            for idx in heatmap_df.index:
                val = heatmap_df.loc[idx, col]
                annot.loc[idx, col] = format_speedup(val)

        # Adjust figure size based on data
        n_patterns = len(valid_patterns)
        n_engines = len(engines)
        self.fig.set_size_inches(n_patterns * 0.8 + 2, n_engines * 0.6 + 1.5)

        # Plot heatmap
        vmin = max(self.vmin, np.nanmin(heatmap_df.values))
        vmax = min(self.vmax, np.nanmax(heatmap_df.values))

        sns.heatmap(
            heatmap_df,
            ax=self.ax,
            annot=annot,
            fmt='',
            cmap=get_heatmap_cmap(),
            center=1.0,
            vmin=vmin,
            vmax=vmax,
            linewidths=PLOT_PARAMS['heatmap_linewidth'],
            linecolor=PLOT_PARAMS['heatmap_linecolor'],
            cbar_kws={'label': 'Speedup vs CTRE', 'shrink': 0.8},
            annot_kws={'size': 10, 'weight': 'medium'},
        )

        self.ax.set_xlabel('')
        self.ax.set_ylabel('')
        plt.xticks(rotation=40, ha='right')
        plt.yticks(rotation=0)

        return self

# ============================================================================
# WORST CASE HEATMAP
# ============================================================================

class WorstCaseHeatmap(BaseFigure):
    """
    Heatmap showing worst-case time differences (SIMD - baseline).
    """

    def __init__(
        self,
        title: str = None,
        simd_engine: str = 'CTRE-SIMD',
        baseline_engine: str = 'CTRE',
    ):
        super().__init__(figsize=FIGURE_SIZES['heatmap_small'], title=title)
        self.simd_engine = simd_engine
        self.baseline_engine = baseline_engine

    def plot(
        self,
        data: BenchmarkData,
        pattern_prefix: str = None,
    ):
        """
        Create worst-case time difference heatmap.

        Shows max(SIMD_time - baseline_time) across all input sizes.
        """
        self.create_figure()

        patterns = sorted(data.patterns)
        if pattern_prefix:
            patterns = [p for p in patterns if p.startswith(pattern_prefix)]

        results = []

        for pat in patterns:
            pat_data = data.filter_pattern(pat)
            simd_data = pat_data[pat_data['Engine'] == self.simd_engine]
            base_data = pat_data[pat_data['Engine'] == self.baseline_engine]

            if simd_data.empty or base_data.empty:
                continue

            # Find worst case (largest time difference)
            max_diff = float('-inf')
            worst_size = None

            for size in data.sizes:
                simd_time = simd_data[simd_data['Input_Size'] == size]['Time_ns'].values
                base_time = base_data[base_data['Input_Size'] == size]['Time_ns'].values

                if len(simd_time) > 0 and len(base_time) > 0:
                    diff = simd_time[0] - base_time[0]
                    if diff > max_diff:
                        max_diff = diff
                        worst_size = size

            if worst_size is not None:
                results.append({
                    'Pattern': pat.split('/')[-1],
                    'Worst_Diff_ns': max_diff,
                    'Size': worst_size,
                })

        if not results:
            return self

        df = pd.DataFrame(results).sort_values('Worst_Diff_ns', ascending=False)

        colors = ['#d73027' if d > 0 else '#1a9850' for d in df['Worst_Diff_ns']]

        y = np.arange(len(df))
        self.ax.barh(y, df['Worst_Diff_ns'], color=colors)
        self.ax.set_yticks(y)
        self.ax.set_yticklabels(df['Pattern'])
        self.ax.set_xlabel('Time Difference (ns): SIMD - Baseline')
        self.ax.axvline(x=0, color='black', linestyle='-', linewidth=0.5)

        return self
