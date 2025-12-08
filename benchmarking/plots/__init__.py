"""Plotting library for benchmark visualization."""
from .style import (ENGINE_COLORS, ENGINE_ORDER, format_size, format_speedup,
                    get_pattern_label, setup_style)
from .loader import (BenchmarkData, load_benchmark_results, load_compile_overhead,
                     load_compile_time, load_csv, merge_simd_baseline)
from .figures import (BarComparison, CompileOverheadPlot, SIMDOverheadPlot,
                      SpeedupHeatmap, SpeedupPlot, TimeSeriesPlot)

__all__ = [
    'setup_style', 'ENGINE_COLORS', 'ENGINE_ORDER', 'format_size', 'format_speedup', 'get_pattern_label',
    'BenchmarkData', 'load_benchmark_results', 'load_compile_overhead', 'load_compile_time', 'load_csv', 'merge_simd_baseline',
    'BarComparison', 'CompileOverheadPlot', 'SIMDOverheadPlot', 'SpeedupHeatmap', 'SpeedupPlot', 'TimeSeriesPlot',
]


