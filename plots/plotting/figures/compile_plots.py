"""
Compilation overhead visualization figures.
"""

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from pathlib import Path
from typing import Optional
from matplotlib.patches import Patch

from .base import BaseFigure
from ..config import FIGURE_SIZES, CATEGORY_COLORS, get_pattern_label

# ============================================================================
# COMPILE OVERHEAD PLOT
# ============================================================================

class CompileOverheadPlot(BaseFigure):
    """
    Visualization of compilation time and binary size overhead.
    """

    def __init__(
        self,
        title: str = None,
    ):
        super().__init__(figsize=FIGURE_SIZES['double'], title=title)

    def plot(
        self,
        df: pd.DataFrame,
    ):
        """
        Create compilation overhead comparison.

        Args:
            df: DataFrame with columns: Pattern, Category, Compile_Time_s, Binary_Size_KB, SIMD
        """
        self.fig, axes = plt.subplots(1, 2, figsize=FIGURE_SIZES['double'])
        self.ax = axes

        categories = ['Simple', 'Complex', 'Behemoth']
        x = np.arange(len(categories))
        width = 0.35

        # Calculate averages
        baseline_times = []
        simd_times = []
        baseline_sizes = []
        simd_sizes = []

        for cat in categories:
            cat_data = df[df['Category'] == cat]
            baseline_times.append(cat_data[cat_data['SIMD'] == 'Baseline']['Compile_Time_s'].mean())
            simd_times.append(cat_data[cat_data['SIMD'] == 'SIMD']['Compile_Time_s'].mean())
            baseline_sizes.append(cat_data[cat_data['SIMD'] == 'Baseline']['Binary_Size_KB'].mean())
            simd_sizes.append(cat_data[cat_data['SIMD'] == 'SIMD']['Binary_Size_KB'].mean())

        # Plot 1: Compile time
        ax1 = axes[0]
        bars1 = ax1.bar(x - width/2, baseline_times, width, label='Baseline', color='#92c5de')
        bars2 = ax1.bar(x + width/2, simd_times, width, label='SIMD', color='#2166ac')

        for bars in [bars1, bars2]:
            for bar in bars:
                height = bar.get_height()
                if height > 0:
                    ax1.annotate(
                        f'{height:.2f}s',
                        xy=(bar.get_x() + bar.get_width()/2, height),
                        xytext=(0, 3),
                        textcoords='offset points',
                        ha='center',
                        va='bottom',
                        fontsize=9,
                    )

        ax1.set_xlabel('Pattern Category')
        ax1.set_ylabel('Average Compile Time (s)')
        ax1.set_title('Compilation Time')
        ax1.set_xticks(x)
        ax1.set_xticklabels(categories)
        ax1.legend()

        # Plot 2: Binary size
        ax2 = axes[1]
        bars3 = ax2.bar(x - width/2, baseline_sizes, width, label='Baseline', color='#92c5de')
        bars4 = ax2.bar(x + width/2, simd_sizes, width, label='SIMD', color='#2166ac')

        for bars in [bars3, bars4]:
            for bar in bars:
                height = bar.get_height()
                if height > 0:
                    ax2.annotate(
                        f'{height:.0f}KB',
                        xy=(bar.get_x() + bar.get_width()/2, height),
                        xytext=(0, 3),
                        textcoords='offset points',
                        ha='center',
                        va='bottom',
                        fontsize=9,
                    )

        ax2.set_xlabel('Pattern Category')
        ax2.set_ylabel('Average Binary Size (KB)')
        ax2.set_title('Binary Size')
        ax2.set_xticks(x)
        ax2.set_xticklabels(categories)
        ax2.legend()

        return self

# ============================================================================
# SIMD OVERHEAD PLOT
# ============================================================================

class SIMDOverheadPlot(BaseFigure):
    """
    Horizontal bar chart showing SIMD overhead percentage per pattern.
    """

    def __init__(
        self,
        title: str = 'SIMD Compilation Overhead',
    ):
        super().__init__(figsize=(10, 8), title=title)

    def plot(
        self,
        df: pd.DataFrame,
    ):
        """
        Create SIMD overhead comparison.

        Args:
            df: DataFrame with compile overhead data
        """
        self.create_figure()

        patterns = df['Pattern'].unique()
        compile_overhead = []
        pattern_names = []
        categories_list = []

        for pat in patterns:
            baseline = df[(df['Pattern'] == pat) & (df['SIMD'] == 'Baseline')]
            simd = df[(df['Pattern'] == pat) & (df['SIMD'] == 'SIMD')]

            if len(baseline) > 0 and len(simd) > 0:
                base_time = baseline['Compile_Time_s'].values[0]
                simd_time = simd['Compile_Time_s'].values[0]

                if base_time > 0:
                    compile_overhead.append((simd_time - base_time) / base_time * 100)
                    pattern_names.append(pat)
                    categories_list.append(baseline['Category'].values[0])

        if not pattern_names:
            return self

        overhead_df = pd.DataFrame({
            'Pattern': pattern_names,
            'Category': categories_list,
            'Overhead': compile_overhead,
        }).sort_values('Overhead')

        colors = [CATEGORY_COLORS.get(c, '#888888') for c in overhead_df['Category']]

        y = np.arange(len(overhead_df))
        self.ax.barh(y, overhead_df['Overhead'], color=colors)
        self.ax.set_yticks(y)
        self.ax.set_yticklabels([get_pattern_label(p) for p in overhead_df['Pattern']])
        self.ax.set_xlabel('SIMD Compile Time Overhead (%)')
        self.ax.axvline(x=0, color='black', linestyle='-', linewidth=0.5)

        # Legend
        legend_elements = [
            Patch(facecolor=CATEGORY_COLORS[c], label=c)
            for c in ['Simple', 'Complex', 'Behemoth']
            if c in set(overhead_df['Category'])
        ]
        self.ax.legend(handles=legend_elements, loc='lower right')

        return self
