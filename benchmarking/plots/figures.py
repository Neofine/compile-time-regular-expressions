"""Figure classes for benchmark visualization."""
import logging
from abc import ABC, abstractmethod
from pathlib import Path
from typing import List, Tuple

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import seaborn as sns
from matplotlib.patches import Patch

from .style import (CATEGORY_COLORS, ENGINE_LABELS, ENGINE_ORDER, ENGINE_ORDER_HEATMAP,
                    ENGINE_STYLES, FIGURE_SIZES, PLOT_PARAMS, format_size, format_speedup,
                    get_heatmap_cmap, get_pattern_label, setup_style)
from .loader import BenchmarkData

logger = logging.getLogger(__name__)


class BaseFigure(ABC):
    def __init__(self, figsize: Tuple[float, float] = None, title: str = None):
        setup_style()
        self.figsize = figsize or FIGURE_SIZES['single']
        self.title = title
        self.fig = self.ax = None

    def create_figure(self, nrows: int = 1, ncols: int = 1):
        self.fig, self.ax = plt.subplots(nrows, ncols, figsize=self.figsize)
        return self.fig, self.ax

    @abstractmethod
    def plot(self, data, **kwargs):
        pass

    def finalize(self):
        if self.title and self.ax is not None:
            if hasattr(self.ax, '__iter__'):
                self.fig.suptitle(self.title, fontsize=13, fontweight='bold')
            else:
                self.ax.set_title(self.title)
        plt.tight_layout()

    def save(self, output_path: Path, close: bool = True):
        output_path = Path(output_path)
        output_path.parent.mkdir(parents=True, exist_ok=True)
        self.finalize()
        self.fig.savefig(output_path, dpi=PLOT_PARAMS['save_dpi'], bbox_inches='tight', facecolor='white')
        if close:
            plt.close(self.fig)
        logger.info(f"Saved: {output_path}")
        return output_path


class TimeSeriesPlot(BaseFigure):
    def __init__(self, title: str = None, xlabel: str = 'Input Size',
                 ylabel: str = 'Matching Time (ns)', log_x: bool = True, log_y: bool = True):
        super().__init__(figsize=FIGURE_SIZES['single'], title=title)
        self.xlabel, self.ylabel = xlabel, ylabel
        self.log_x, self.log_y = log_x, log_y

    def plot(self, data: BenchmarkData, pattern: str, engines: List[str] = None):
        self.create_figure()
        engines = engines or ENGINE_ORDER
        pattern_data = data.filter_pattern(pattern)
        sizes = sorted(pattern_data['Input_Size'].unique())

        for engine in engines:
            eng_data = pattern_data[pattern_data['Engine'] == engine].sort_values('Input_Size')
            if eng_data.empty:
                continue
            style = ENGINE_STYLES.get(engine, ENGINE_STYLES['CTRE'])
            self.ax.plot(eng_data['Input_Size'], eng_data['Time_ns'],
                         marker=style['marker'], linestyle=style['linestyle'],
                         color=style['color'], linewidth=style['linewidth'],
                         markersize=style['markersize'],
                         label=ENGINE_LABELS.get(engine, engine), zorder=style['zorder'])

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


class SpeedupPlot(BaseFigure):
    def __init__(self, title: str = None, baseline_engine: str = 'CTRE',
                 xlabel: str = 'Input Size', ylabel: str = 'Speedup vs Baseline'):
        super().__init__(figsize=FIGURE_SIZES['single'], title=title)
        self.baseline_engine = baseline_engine
        self.xlabel, self.ylabel = xlabel, ylabel

    def plot(self, data: BenchmarkData, pattern: str, engines: List[str] = None):
        self.create_figure()
        engines = engines or [e for e in ENGINE_ORDER if e != self.baseline_engine]
        pattern_data = data.filter_pattern(pattern)
        sizes = sorted(pattern_data['Input_Size'].unique())
        baseline_data = pattern_data[pattern_data['Engine'] == self.baseline_engine]
        baseline_times = dict(zip(baseline_data['Input_Size'], baseline_data['Time_ns']))

        for engine in engines:
            eng_data = pattern_data[pattern_data['Engine'] == engine]
            if eng_data.empty:
                continue
            speedups, valid_sizes = [], []
            for _, row in eng_data.iterrows():
                if row['Input_Size'] in baseline_times and baseline_times[row['Input_Size']] > 0:
                    speedups.append(baseline_times[row['Input_Size']] / row['Time_ns'])
                    valid_sizes.append(row['Input_Size'])
            if speedups:
                style = ENGINE_STYLES.get(engine, ENGINE_STYLES['CTRE'])
                self.ax.plot(valid_sizes, speedups, marker=style['marker'],
                             linestyle=style['linestyle'], color=style['color'],
                             linewidth=style['linewidth'], markersize=style['markersize'],
                             label=ENGINE_LABELS.get(engine, engine), zorder=style['zorder'])

        self.ax.axhline(y=1.0, color='#333333', linestyle='--', linewidth=1, alpha=0.7)
        self.ax.set_xlabel(self.xlabel)
        self.ax.set_ylabel(self.ylabel)
        self.ax.set_xscale('log', base=2)
        self.ax.set_xticks(sizes)
        self.ax.set_xticklabels([format_size(s) for s in sizes])
        self.ax.legend(loc='best', frameon=True)
        return self


class SpeedupHeatmap(BaseFigure):
    def __init__(self, title: str = None, baseline_engine: str = 'CTRE',
                 vmin: float = 0.1, vmax: float = 10.0):
        super().__init__(figsize=FIGURE_SIZES['heatmap_large'], title=title)
        self.baseline_engine = baseline_engine
        self.vmin, self.vmax = vmin, vmax

    def plot(self, data: BenchmarkData, patterns: list = None, engines: list = None,
             size: int = 1024, pattern_prefix: str = None, exclude_patterns: list = None):
        self.create_figure()
        available_engines = [e for e in data.engines if e != self.baseline_engine]
        if engines:
            engines = [e for e in engines if e in available_engines]
        else:
            engines = [e for e in ENGINE_ORDER_HEATMAP if e in available_engines]
            engines.extend(e for e in available_engines if e not in engines)

        if not engines:
            return self

        size_data = data.filter_size(size)
        all_patterns = sorted(data.patterns)
        if pattern_prefix:
            all_patterns = [p for p in all_patterns if p.startswith(pattern_prefix)]
        if exclude_patterns:
            all_patterns = [p for p in all_patterns if not any(ex in p for ex in exclude_patterns)]
        patterns = patterns or all_patterns

        speedup_data, valid_patterns = {}, []
        for pat in patterns:
            pat_data = size_data[size_data['Pattern'] == pat]
            baseline = pat_data[pat_data['Engine'] == self.baseline_engine]['Time_ns'].values
            if len(baseline) == 0 or baseline[0] <= 0:
                continue
            baseline_time = baseline[0]
            pat_label = get_pattern_label(pat.split('/')[-1])
            valid_patterns.append(pat_label)
            for eng in engines:
                speedup_data.setdefault(eng, {})
                eng_time = pat_data[pat_data['Engine'] == eng]['Time_ns'].values
                speedup_data[eng][pat_label] = baseline_time / eng_time[0] if len(eng_time) > 0 and eng_time[0] > 0 else np.nan

        if not valid_patterns:
            return self

        heatmap_df = pd.DataFrame(speedup_data).T.reindex(engines)[valid_patterns]
        annot = heatmap_df.copy().astype(object)
        for col in heatmap_df.columns:
            for idx in heatmap_df.index:
                annot.loc[idx, col] = format_speedup(heatmap_df.loc[idx, col])

        self.fig.set_size_inches(len(valid_patterns) * 0.8 + 2, len(engines) * 0.6 + 1.5)
        sns.heatmap(heatmap_df, ax=self.ax, annot=annot, fmt='',
                    cmap=get_heatmap_cmap(), center=1.0,
                    vmin=max(self.vmin, np.nanmin(heatmap_df.values)),
                    vmax=min(self.vmax, np.nanmax(heatmap_df.values)),
                    linewidths=PLOT_PARAMS['heatmap_linewidth'],
                    linecolor=PLOT_PARAMS['heatmap_linecolor'],
                    cbar_kws={'label': 'Speedup vs CTRE', 'shrink': 0.8},
                    annot_kws={'size': 10, 'weight': 'medium'})
        self.ax.set_xlabel('')
        self.ax.set_ylabel('')
        plt.xticks(rotation=40, ha='right')
        plt.yticks(rotation=0)
        return self


class BarComparison(BaseFigure):
    def __init__(self, title: str = None, ylabel: str = 'Matching Time (ns)',
                 log_scale: bool = True, show_speedup: bool = False, baseline_engine: str = 'CTRE'):
        super().__init__(figsize=FIGURE_SIZES['wide'], title=title)
        self.ylabel, self.log_scale = ylabel, log_scale
        self.show_speedup, self.baseline_engine = show_speedup, baseline_engine

    def plot(self, data: BenchmarkData, patterns: List[str], labels: List[str] = None,
             engines: List[str] = None, size: int = 1024):
        self.create_figure()
        labels = labels or [p.split('/')[-1] for p in patterns]
        engines = engines or [e for e in ENGINE_ORDER if e != self.baseline_engine]
        size_data = data.filter_size(size)
        x, width = np.arange(len(patterns)), PLOT_PARAMS['bar_width']

        baseline_times = {}
        if self.show_speedup:
            for pat in patterns:
                pat_data = size_data[(size_data['Pattern'] == pat) & (size_data['Engine'] == self.baseline_engine)]
                if not pat_data.empty:
                    baseline_times[pat] = pat_data['Time_ns'].values[0]

        for i, engine in enumerate(engines):
            values = []
            for pat in patterns:
                pat_data = size_data[(size_data['Pattern'] == pat) & (size_data['Engine'] == engine)]
                if pat_data.empty:
                    values.append(0)
                elif self.show_speedup and pat in baseline_times:
                    values.append(baseline_times[pat] / pat_data['Time_ns'].values[0])
                else:
                    values.append(pat_data['Time_ns'].values[0])

            style = ENGINE_STYLES.get(engine, ENGINE_STYLES['CTRE'])
            self.ax.bar(x + (i - len(engines)/2 + 0.5) * width, values, width,
                        label=ENGINE_LABELS.get(engine, engine), color=style['color'],
                        edgecolor=PLOT_PARAMS['bar_edge_color'], linewidth=PLOT_PARAMS['bar_edge_width'], zorder=3)

        if self.show_speedup:
            self.ax.axhline(y=1.0, color='#333333', linestyle='-', linewidth=1, zorder=1)

        self.ax.set_xticks(x)
        self.ax.set_xticklabels(labels, rotation=30, ha='right')
        self.ax.set_ylabel(self.ylabel)
        if self.log_scale:
            self.ax.set_yscale('log')
        self.ax.legend(loc='upper right', fontsize=8)
        self.ax.set_axisbelow(True)
        return self


class CompileOverheadPlot(BaseFigure):
    # Map internal category names to display names
    CATEGORY_DISPLAY = {'Behemoth': 'ExtraLarge', 'Simple': 'Simple', 'Complex': 'Complex'}
    
    def __init__(self, title: str = None):
        super().__init__(figsize=FIGURE_SIZES['double'], title=title)

    def plot(self, df: pd.DataFrame):
        self.fig, axes = plt.subplots(1, 2, figsize=FIGURE_SIZES['double'])
        self.ax = axes
        categories_data = ['Simple', 'Complex', 'Behemoth']  # Names in CSV
        categories_display = [self.CATEGORY_DISPLAY.get(c, c) for c in categories_data]
        x, width = np.arange(len(categories_data)), 0.35

        baseline_times = [df[(df['Category'] == c) & (df['SIMD'] == 'Baseline')]['Compile_Time_s'].mean() for c in categories_data]
        simd_times = [df[(df['Category'] == c) & (df['SIMD'] == 'SIMD')]['Compile_Time_s'].mean() for c in categories_data]
        baseline_sizes = [df[(df['Category'] == c) & (df['SIMD'] == 'Baseline')]['Binary_Size_KB'].mean() for c in categories_data]
        simd_sizes = [df[(df['Category'] == c) & (df['SIMD'] == 'SIMD')]['Binary_Size_KB'].mean() for c in categories_data]

        for ax, (base, simd, ylabel, title) in zip(axes, [
            (baseline_times, simd_times, 'Compile Time (s)', 'Compilation'),
            (baseline_sizes, simd_sizes, 'Binary Size (KB)', 'Binary Size')
        ]):
            ax.bar(x - width/2, base, width, label='Baseline', color='#92c5de')
            ax.bar(x + width/2, simd, width, label='SIMD', color='#2166ac')
            ax.set_xlabel('Category')
            ax.set_ylabel(ylabel)
            ax.set_title(title)
            ax.set_xticks(x)
            ax.set_xticklabels(categories_display)
            ax.legend()
        return self


class SIMDOverheadPlot(BaseFigure):
    CATEGORY_DISPLAY = {'Behemoth': 'ExtraLarge', 'Simple': 'Simple', 'Complex': 'Complex'}
    
    def __init__(self, title: str = 'SIMD Compilation Overhead'):
        super().__init__(figsize=(10, 8), title=title)

    def plot(self, df: pd.DataFrame):
        self.create_figure()
        overhead, names, cats = [], [], []
        for pat in df['Pattern'].unique():
            base = df[(df['Pattern'] == pat) & (df['SIMD'] == 'Baseline')]
            simd = df[(df['Pattern'] == pat) & (df['SIMD'] == 'SIMD')]
            if len(base) > 0 and len(simd) > 0 and base['Compile_Time_s'].values[0] > 0:
                overhead.append((simd['Compile_Time_s'].values[0] - base['Compile_Time_s'].values[0]) / base['Compile_Time_s'].values[0] * 100)
                names.append(pat)
                cats.append(base['Category'].values[0])

        if not names:
            return self

        odf = pd.DataFrame({'Pattern': names, 'Category': cats, 'Overhead': overhead}).sort_values('Overhead')
        colors = [CATEGORY_COLORS.get(c, '#888') for c in odf['Category']]
        y = np.arange(len(odf))
        self.ax.barh(y, odf['Overhead'], color=colors)
        self.ax.set_yticks(y)
        self.ax.set_yticklabels([get_pattern_label(p) for p in odf['Pattern']])
        self.ax.set_xlabel('SIMD Overhead (%)')
        self.ax.axvline(x=0, color='black', linewidth=0.5)
        display_cats = ['Simple', 'Complex', 'ExtraLarge']
        data_cats = ['Simple', 'Complex', 'Behemoth']
        legend = [Patch(facecolor=CATEGORY_COLORS[dc], label=dc) for dc, c in zip(display_cats, data_cats) if c in set(cats)]
        self.ax.legend(handles=legend, loc='lower right')
        return self

