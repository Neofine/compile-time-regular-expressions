"""Data loading utilities for benchmark results."""
import logging
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Optional

import pandas as pd

logger = logging.getLogger(__name__)


@dataclass
class BenchmarkData:
    """Container for benchmark data."""
    df: pd.DataFrame
    category: str
    source_files: List[Path]

    @property
    def patterns(self) -> List[str]:
        return sorted(self.df['Pattern'].unique().tolist())

    @property
    def engines(self) -> List[str]:
        return sorted(self.df['Engine'].unique().tolist())

    @property
    def sizes(self) -> List[int]:
        return sorted(self.df['Input_Size'].unique().tolist())

    def filter_size(self, size: int) -> pd.DataFrame:
        return self.df[self.df['Input_Size'] == size].copy()

    def filter_pattern(self, pattern: str) -> pd.DataFrame:
        return self.df[self.df['Pattern'] == pattern].copy()

    def filter_engine(self, engine: str) -> pd.DataFrame:
        return self.df[self.df['Engine'] == engine].copy()

    def get_time(self, pattern: str, engine: str, size: int) -> Optional[float]:
        mask = (self.df['Pattern'] == pattern) & (self.df['Engine'] == engine) & (self.df['Input_Size'] == size)
        values = self.df.loc[mask, 'Time_ns'].values
        return values[0] if len(values) > 0 else None


def load_csv(path: Path) -> Optional[pd.DataFrame]:
    """Load and validate a benchmark CSV file."""
    if not path.exists():
        logger.warning(f"File not found: {path}")
        return None

    try:
        with open(path) as f:
            lines = f.readlines()

        skip_rows = next((i for i, line in enumerate(lines) if line.strip().startswith('Pattern,')), 0)
        df = pd.read_csv(path, skiprows=skip_rows)

        required = ['Pattern', 'Engine', 'Input_Size', 'Time_ns']
        if missing := [c for c in required if c not in df.columns]:
            logger.warning(f"Missing columns in {path}: {missing}")
            return None

        df['Input_Size'] = pd.to_numeric(df['Input_Size'], errors='coerce')
        df['Time_ns'] = pd.to_numeric(df['Time_ns'], errors='coerce')

        for col in ['Time_std', 'Time_min', 'Time_max', 'Time_median', 'Time_ci_lower', 'Time_ci_upper']:
            if col in df.columns:
                df[col] = pd.to_numeric(df[col], errors='coerce')

        df = df.dropna(subset=['Input_Size', 'Time_ns'])
        df = df[df['Time_ns'] > 0]
        df['Input_Size'] = df['Input_Size'].astype(int)
        df = df[~df['Pattern'].str.contains('Pattern', na=False)]

        return df
    except Exception as e:
        logger.error(f"Error loading {path}: {e}")
        return None


def merge_benchmark_csvs(*csv_paths: Path) -> Optional[pd.DataFrame]:
    """Merge multiple benchmark CSV files."""
    dfs = []
    for path in csv_paths:
        if path.exists():
            df = load_csv(path)
            if df is not None and not df.empty:
                dfs.append(df)
                logger.info(f"Loaded {path.name}: {len(df)} rows")

    if not dfs:
        return None

    merged = pd.concat(dfs, ignore_index=True)
    return merged.drop_duplicates(subset=['Pattern', 'Engine', 'Input_Size'], keep='first')


def merge_simd_baseline(simd_path: Path, baseline_path: Path) -> Optional[pd.DataFrame]:
    """Merge SIMD and baseline CSV files."""
    return merge_benchmark_csvs(simd_path, baseline_path)


def load_benchmark_results(base_dir: Path, category: str, simd_file: str = 'simd.csv',
                           baseline_file: str = 'baseline.csv') -> Optional[BenchmarkData]:
    """Load benchmark results from a category directory."""
    cat_dir = base_dir / ('arm' if category == 'arm_nomatch' else category)

    if cat_dir.exists():
        results_path = cat_dir / 'results.csv'
        if results_path.exists():
            df = load_csv(results_path)
            if df is not None and not df.empty:
                return BenchmarkData(df=df, category=category, source_files=[results_path])

        csv_files = [cat_dir / f for f in [simd_file, 'scalar.csv', 'original.csv', baseline_file]]
        existing = [f for f in csv_files if f.exists()]
        if existing:
            df = merge_benchmark_csvs(*existing)
            if df is not None and not df.empty:
                return BenchmarkData(df=df, category=category, source_files=existing)

    simd_path, baseline_path = base_dir / simd_file, base_dir / baseline_file
    if simd_path.exists() or baseline_path.exists():
        df = merge_simd_baseline(simd_path, baseline_path)
        if df is not None and not df.empty:
            return BenchmarkData(df=df, category=category,
                                 source_files=[p for p in [simd_path, baseline_path] if p.exists()])

    results_path = base_dir / 'results.csv'
    if results_path.exists():
        df = load_csv(results_path)
        if df is not None and not df.empty:
            return BenchmarkData(df=df, category=category, source_files=[results_path])

    return None


def load_compile_overhead(base_dir: Path) -> Optional[pd.DataFrame]:
    """Load compilation overhead benchmark data."""
    plots_root = Path(__file__).parent.parent.parent
    paths = [
        base_dir / 'compile_overhead' / 'compile_overhead.csv',
        base_dir / 'compile_overhead' / 'results' / 'compile_overhead.csv',
        plots_root / 'output' / 'thesis' / 'compile_overhead' / 'compile_overhead.csv',
    ]

    for path in paths:
        if path.exists():
            try:
                df = pd.read_csv(path)
                return df[df['Compile_Time_s'] > 0]
            except Exception as e:
                logger.error(f"Error loading compile overhead: {e}")
    return None


def load_compile_time(base_dir: Path) -> Optional[pd.DataFrame]:
    """Load compilation time benchmark data."""
    plots_root = Path(__file__).parent.parent.parent
    paths = [base_dir / 'compile_time.csv', plots_root / 'output' / 'compile_time.csv']

    for path in paths:
        if path.exists():
            try:
                df = pd.read_csv(path)
                return df[df['Compile_Time_s'] > 0]
            except Exception as e:
                logger.error(f"Error loading compile time: {e}")
    return None


def compute_speedups(data: BenchmarkData, baseline_engine: str = 'CTRE',
                     size: int = 1024) -> Dict[str, Dict[str, float]]:
    """Compute speedup ratios relative to baseline engine."""
    speedups = {}
    for pattern in data.patterns:
        baseline_time = data.get_time(pattern, baseline_engine, size)
        if not baseline_time or baseline_time <= 0:
            continue

        for engine in data.engines:
            if engine == baseline_engine:
                continue
            if (eng_time := data.get_time(pattern, engine, size)) and eng_time > 0:
                speedups.setdefault(engine, {})[pattern] = baseline_time / eng_time

    return speedups
