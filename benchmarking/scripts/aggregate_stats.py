#!/usr/bin/env python3
"""Aggregate benchmark statistics from multiple runs."""
import csv
import io
import numpy as np
import sys
from pathlib import Path
from collections import defaultdict

# Try to import scipy for confidence intervals
try:
    from scipy import stats
    HAS_SCIPY = True
except ImportError:
    HAS_SCIPY = False

def aggregate_stats(temp_file: Path, final_file: Path):
    """Aggregate statistics from multiple benchmark runs."""
    if not temp_file.exists() or temp_file.stat().st_size == 0:
        print(f"ERROR: No data in {temp_file}", file=sys.stderr)
        sys.exit(1)

    # Read all runs
    with open(temp_file, 'r') as f:
        lines = f.readlines()
    
    # Find header line
    header_idx = 0
    for i, line in enumerate(lines):
        if line.strip().startswith('Pattern,'):
            header_idx = i
            break

    # Parse all runs
    runs = []
    current_run = []
    for line in lines[header_idx:]:
        line = line.strip()
        if line.startswith('Running category:'):
            if current_run:
                runs.append(current_run)
            current_run = []
        elif line and not line.startswith('Pattern,'):
            current_run.append(line)

    if current_run:
        runs.append(current_run)

    if not runs:
        print(f"ERROR: No valid runs found in {temp_file}", file=sys.stderr)
        sys.exit(1)

    if len(runs) < 2:
        print(f"WARNING: Only {len(runs)} run(s) found, statistics may be unreliable", file=sys.stderr)

    # Aggregate statistics
    aggregated = defaultdict(lambda: {'times': [], 'matches': []})

    for run in runs:
        # Use CSV reader to handle commas in pattern names
        # Convert list of strings to file-like object
        import io
        run_text = '\n'.join(run)
        reader = csv.reader(io.StringIO(run_text))
        for row in reader:
            if len(row) < 5:
                continue
            try:
                pattern, engine, size = row[0], row[1], row[2]
                time_ns = float(row[3])
                matches = int(row[4])
                
                # Validate data
                if time_ns <= 0 or time_ns > 1e9:
                    continue
                
                key = f"{pattern},{engine},{size}"
                aggregated[key]['times'].append(time_ns)
                aggregated[key]['matches'].append(matches)
            except (ValueError, IndexError):
                continue

    # Write aggregated results
    with open(final_file, 'w') as f:
        f.write("Pattern,Engine,Input_Size,Time_ns,Time_std,Time_min,Time_max,Time_median,Time_ci_lower,Time_ci_upper,Matches\n")
        
        for key, data in sorted(aggregated.items()):
            times = data['times']
            matches = data['matches']
            
            if len(times) == 0:
                continue
            
            mean_time = np.mean(times)
            std_time = np.std(times, ddof=1) if len(times) > 1 else 0.0
            min_time = np.min(times)
            max_time = np.max(times)
            median_time = np.median(times)
            mean_matches = int(np.mean(matches)) if len(matches) > 0 else 0
            
            # Compute 95% confidence interval
            if len(times) > 1:
                if HAS_SCIPY:
                    try:
                        ci = stats.t.interval(0.95, len(times)-1, loc=mean_time, scale=stats.sem(times))
                        ci_lower = max(0.0, ci[0])
                        ci_upper = ci[1]
                    except (ValueError, RuntimeError):
                        se = std_time / np.sqrt(len(times))
                        ci_lower = max(0.0, mean_time - 1.96 * se)
                        ci_upper = mean_time + 1.96 * se
                else:
                    se = std_time / np.sqrt(len(times))
                    ci_lower = max(0.0, mean_time - 1.96 * se)
                    ci_upper = mean_time + 1.96 * se
            else:
                ci_lower = ci_upper = mean_time
            
            f.write(f"{key},{mean_time:.2f},{std_time:.2f},{min_time:.2f},{max_time:.2f},{median_time:.2f},{ci_lower:.2f},{ci_upper:.2f},{mean_matches}\n")

    print(f"✓ Aggregated {len(runs)} runs → {final_file}")
    if aggregated:
        all_means = [np.mean(aggregated[k]['times']) for k in aggregated]
        all_stds = [np.std(aggregated[k]['times'], ddof=1) if len(aggregated[k]['times']) > 1 else 0.0 for k in aggregated]
        print(f"  Mean time: {np.mean(all_means):.2f} ns")
        print(f"  Avg std dev: {np.mean(all_stds):.2f} ns")
        cv = np.mean([s/m if m > 0 else 0 for s, m in zip(all_stds, all_means)]) * 100
        print(f"  CV (coefficient of variation): {cv:.2f}%")

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <temp_file> <final_file>", file=sys.stderr)
        sys.exit(1)
    
    aggregate_stats(Path(sys.argv[1]), Path(sys.argv[2]))

