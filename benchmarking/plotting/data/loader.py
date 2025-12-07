"""
Data loading utilities for benchmark results.
Handles CSV parsing, merging SIMD/baseline data, and data validation.
"""

import pandas as pd
from pathlib import Path
from dataclasses import dataclass
from typing import Optional, List, Dict
import logging

logger = logging.getLogger(__name__)

# ============================================================================
# DATA STRUCTURES
# ============================================================================

@dataclass
class BenchmarkData:
    """Container for benchmark data with metadata."""
    df: pd.DataFrame
    category: str
    source_files: List[Path]

    @property
    def patterns(self) -> List[str]:
        """Get list of unique patterns."""
        return sorted(self.df['Pattern'].unique().tolist())

    @property
    def engines(self) -> List[str]:
        """Get list of unique engines."""
        return sorted(self.df['Engine'].unique().tolist())

    @property
    def sizes(self) -> List[int]:
        """Get list of unique input sizes."""
        return sorted(self.df['Input_Size'].unique().tolist())

    def filter_size(self, size: int) -> pd.DataFrame:
        """Get data for specific input size."""
        return self.df[self.df['Input_Size'] == size].copy()

    def filter_pattern(self, pattern: str) -> pd.DataFrame:
        """Get data for specific pattern."""
        return self.df[self.df['Pattern'] == pattern].copy()

    def filter_engine(self, engine: str) -> pd.DataFrame:
        """Get data for specific engine."""
        return self.df[self.df['Engine'] == engine].copy()

    def get_time(self, pattern: str, engine: str, size: int) -> Optional[float]:
        """Get time for specific pattern/engine/size combination."""
        mask = (
            (self.df['Pattern'] == pattern) &
            (self.df['Engine'] == engine) &
            (self.df['Input_Size'] == size)
        )
        values = self.df.loc[mask, 'Time_ns'].values
        return values[0] if len(values) > 0 else None

# ============================================================================
# LOADING FUNCTIONS
# ============================================================================

def load_csv(path: Path) -> Optional[pd.DataFrame]:
    """Load and validate a benchmark CSV file.
    
    Supports both standard format (Time_ns) and statistical format (Time_ns, Time_std, Time_min, Time_max).
    """
    if not path.exists():
        logger.warning(f"File not found: {path}")
        return None

    try:
        # Read raw file to find actual CSV header (skip non-CSV lines like "Running category:")
        with open(path, 'r') as f:
            lines = f.readlines()
        
        # Find the header line (should contain "Pattern,Engine,Input_Size,Time_ns")
        skip_rows = 0
        for i, line in enumerate(lines):
            if line.strip().startswith('Pattern,'):
                skip_rows = i
                break
        
        df = pd.read_csv(path, skiprows=skip_rows)

        # Validate required columns
        required_cols = ['Pattern', 'Engine', 'Input_Size', 'Time_ns']
        missing = [c for c in required_cols if c not in df.columns]
        if missing:
            logger.warning(f"Missing columns in {path}: {missing}")
            return None

        # Convert types
        df['Input_Size'] = pd.to_numeric(df['Input_Size'], errors='coerce')
        df['Time_ns'] = pd.to_numeric(df['Time_ns'], errors='coerce')
        
        # Handle statistical columns if present
        if 'Time_std' in df.columns:
            df['Time_std'] = pd.to_numeric(df['Time_std'], errors='coerce')
        if 'Time_min' in df.columns:
            df['Time_min'] = pd.to_numeric(df['Time_min'], errors='coerce')
        if 'Time_max' in df.columns:
            df['Time_max'] = pd.to_numeric(df['Time_max'], errors='coerce')
        if 'Time_median' in df.columns:
            df['Time_median'] = pd.to_numeric(df['Time_median'], errors='coerce')
        if 'Time_ci_lower' in df.columns:
            df['Time_ci_lower'] = pd.to_numeric(df['Time_ci_lower'], errors='coerce')
        if 'Time_ci_upper' in df.columns:
            df['Time_ci_upper'] = pd.to_numeric(df['Time_ci_upper'], errors='coerce')

        # Filter invalid rows
        df = df.dropna(subset=['Input_Size', 'Time_ns'])
        df = df[df['Time_ns'] > 0]
        df['Input_Size'] = df['Input_Size'].astype(int)

        # Remove duplicate header rows
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
            if df is not None:
                dfs.append(df)
                logger.info(f"Loaded {path.name}: {len(df)} rows")

    if not dfs:
        return None

    merged = pd.concat(dfs, ignore_index=True)

    # Remove exact duplicates, keeping first
    merged = merged.drop_duplicates(
        subset=['Pattern', 'Engine', 'Input_Size'],
        keep='first'
    )

    return merged

def merge_simd_baseline(simd_path: Path, baseline_path: Path) -> Optional[pd.DataFrame]:
    """Merge SIMD and baseline CSV files (legacy compatibility)."""
    return merge_benchmark_csvs(simd_path, baseline_path)

def load_benchmark_results(
    base_dir: Path,
    category: str,
    simd_file: str = 'simd.csv',
    baseline_file: str = 'baseline.csv'
) -> Optional[BenchmarkData]:
    """
    Load benchmark results from a category directory.

    Args:
        base_dir: Base output directory (e.g., benchmarking/output)
        category: Category name (e.g., 'simple', 'complex', 'scaling')
        simd_file: Name of SIMD results file
        baseline_file: Name of baseline results file

    Returns:
        BenchmarkData object or None if loading fails
    """
    # Try category subdirectory first (prefer category-specific files)
    cat_dir = base_dir / category
    
    # For arm_nomatch, data is in the arm/ folder with arm_nomatch/ pattern prefix
    if category == 'arm_nomatch':
        cat_dir = base_dir / 'arm'
    if cat_dir.exists():
        # Try merged results.csv first
        results_path = cat_dir / 'results.csv'
        if results_path.exists():
            df = load_csv(results_path)
            if df is not None:
                return BenchmarkData(
                    df=df,
                    category=category,
                    source_files=[results_path]
                )

        # Try all CTRE variant files (simd, scalar, original)
        csv_files = [
            cat_dir / simd_file,
            cat_dir / 'scalar.csv',
            cat_dir / 'original.csv',
            cat_dir / baseline_file,
        ]
        existing_files = [f for f in csv_files if f.exists()]
        if existing_files:
            df = merge_benchmark_csvs(*existing_files)
            if df is not None:
                return BenchmarkData(
                    df=df,
                    category=category,
                    source_files=existing_files
                )

    # Fall back to root-level merged files (for overview/legacy support)
    simd_path = base_dir / simd_file
    baseline_path = base_dir / baseline_file
    if simd_path.exists() or baseline_path.exists():
        df = merge_simd_baseline(simd_path, baseline_path)
        if df is not None:
            source_files = [p for p in [simd_path, baseline_path] if p.exists()]
            return BenchmarkData(
                df=df,
                category=category,
                source_files=source_files
            )

    # Fall back to results.csv in base_dir
    results_path = base_dir / 'results.csv'
    if results_path.exists():
        df = load_csv(results_path)
        if df is not None:
            return BenchmarkData(
                df=df,
                category=category,
                source_files=[results_path]
            )

    return None

def load_compile_overhead(base_dir: Path) -> Optional[pd.DataFrame]:
    """Load compilation overhead benchmark data."""
    # Get benchmarking root directory (plotting/data/loader.py -> plotting -> benchmarking)
    plots_root = Path(__file__).parent.parent.parent

    # Try multiple possible locations
    possible_paths = [
        base_dir / 'compile_overhead' / 'compile_overhead.csv',
        base_dir / 'compile_overhead' / 'results' / 'compile_overhead.csv',
        plots_root / 'output' / 'thesis' / 'compile_overhead' / 'compile_overhead.csv',
    ]

    csv_path = None
    for path in possible_paths:
        if path.exists():
            csv_path = path
            break

    if csv_path is None:
        logger.warning(f"Compile overhead data not found")
        return None

    try:
        df = pd.read_csv(csv_path)
        df = df[df['Compile_Time_s'] > 0]  # Filter failed compilations
        return df
    except Exception as e:
        logger.error(f"Error loading compile overhead: {e}")
        return None

def load_compile_time(base_dir: Path) -> Optional[pd.DataFrame]:
    """Load compilation time benchmark data."""
    plots_root = Path(__file__).parent.parent.parent

    possible_paths = [
        base_dir / 'compile_time.csv',
        plots_root / 'output' / 'compile_time.csv',
    ]

    csv_path = None
    for path in possible_paths:
        if path.exists():
            csv_path = path
            break

    if csv_path is None:
        logger.warning(f"Compile time data not found")
        return None

    try:
        df = pd.read_csv(csv_path)
        # Filter invalid rows
        df = df[df['Compile_Time_s'] > 0]
        return df
    except Exception as e:
        logger.error(f"Error loading compile time: {e}")
        return None

# ============================================================================
# DATA ANALYSIS UTILITIES
# ============================================================================

def compute_speedups(
    data: BenchmarkData,
    baseline_engine: str = 'CTRE',
    size: int = 1024
) -> Dict[str, Dict[str, float]]:
    """
    Compute speedup ratios relative to baseline engine.

    Returns:
        Dict[engine][pattern] = speedup_ratio
    """
    size_data = data.filter_size(size)
    speedups = {}

    for pattern in data.patterns:
        baseline_time = data.get_time(pattern, baseline_engine, size)
        if baseline_time is None or baseline_time <= 0:
            continue

        for engine in data.engines:
            if engine == baseline_engine:
                continue

            eng_time = data.get_time(pattern, engine, size)
            if eng_time is None or eng_time <= 0:
                continue

            if engine not in speedups:
                speedups[engine] = {}

            speedups[engine][pattern] = baseline_time / eng_time

    return speedups
