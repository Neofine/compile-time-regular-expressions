#!/usr/bin/env python3
import csv
import io
import sys
from collections import defaultdict
from pathlib import Path

import numpy as np

try:
    from scipy import stats
    HAS_SCIPY = True
except ImportError:
    HAS_SCIPY = False


def aggregate_stats(temp_file: Path, final_file: Path):
    if not temp_file.exists() or temp_file.stat().st_size == 0:
        print(f"ERROR: No data in {temp_file}", file=sys.stderr)
        sys.exit(1)

    with open(temp_file) as f:
        lines = f.readlines()

    header_idx = next((i for i, line in enumerate(lines) if line.strip().startswith('Pattern,')), 0)

    runs, current_run = [], []
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
        print(f"WARNING: Only {len(runs)} run(s) found", file=sys.stderr)

    aggregated = defaultdict(lambda: {'times': [], 'matches': []})

    for run in runs:
        reader = csv.reader(io.StringIO('\n'.join(run)))
        for row in reader:
            if len(row) < 5:
                continue
            try:
                pattern, engine, size = row[0], row[1], row[2]
                time_ns, matches = float(row[3]), int(row[4])
                if 0 < time_ns <= 1e9:
                    key = f"{pattern},{engine},{size}"
                    aggregated[key]['times'].append(time_ns)
                    aggregated[key]['matches'].append(matches)
            except (ValueError, IndexError):
                continue

    with open(final_file, 'w') as f:
        f.write("Pattern,Engine,Input_Size,Time_ns,Time_std,Time_min,Time_max,Time_median,Time_ci_lower,Time_ci_upper,Matches\n")
        
        for key, data in sorted(aggregated.items()):
            times = data['times']
            if not times:
                continue

            mean_time = np.mean(times)
            std_time = np.std(times, ddof=1) if len(times) > 1 else 0.0
            min_time, max_time, median_time = np.min(times), np.max(times), np.median(times)
            mean_matches = int(np.mean(data['matches'])) if data['matches'] else 0

            if len(times) > 1:
                if HAS_SCIPY:
                    try:
                        ci = stats.t.interval(0.95, len(times)-1, loc=mean_time, scale=stats.sem(times))
                        ci_lower, ci_upper = max(0.0, ci[0]), ci[1]
                    except (ValueError, RuntimeError):
                        se = std_time / np.sqrt(len(times))
                        ci_lower, ci_upper = max(0.0, mean_time - 1.96*se), mean_time + 1.96*se
                else:
                    se = std_time / np.sqrt(len(times))
                    ci_lower, ci_upper = max(0.0, mean_time - 1.96*se), mean_time + 1.96*se
            else:
                ci_lower = ci_upper = mean_time

            f.write(f"{key},{mean_time:.2f},{std_time:.2f},{min_time:.2f},{max_time:.2f},"
                    f"{median_time:.2f},{ci_lower:.2f},{ci_upper:.2f},{mean_matches}\n")

    print(f"✓ Aggregated {len(runs)} runs → {final_file}")
    if aggregated:
        all_means = [np.mean(aggregated[k]['times']) for k in aggregated]
        all_stds = [np.std(aggregated[k]['times'], ddof=1) if len(aggregated[k]['times']) > 1 else 0.0 for k in aggregated]
        cv = np.mean([s/m if m > 0 else 0 for s, m in zip(all_stds, all_means)]) * 100
        print(f"  Mean: {np.mean(all_means):.2f} ns, Std: {np.mean(all_stds):.2f} ns, CV: {cv:.2f}%")


if __name__ == '__main__':
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <temp_file> <final_file>", file=sys.stderr)
        sys.exit(1)
    aggregate_stats(Path(sys.argv[1]), Path(sys.argv[2]))
