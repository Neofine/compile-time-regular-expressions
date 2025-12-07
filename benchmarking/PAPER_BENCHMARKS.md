# Publication-Quality Benchmark Setup

This guide describes how to run benchmarks suitable for academic publication with proper CPU isolation, NUMA pinning, and statistical analysis.

## Overview

For publication-quality results, we need to:
1. **Minimize system noise**: CPU isolation, disable frequency scaling, disable turbo boost
2. **Ensure reproducibility**: NUMA pinning, consistent CPU assignment
3. **Statistical rigor**: Multiple runs with mean, standard deviation, confidence intervals
4. **Document system state**: Record CPU frequencies, kernel version, compiler version

## Prerequisites

- Root access (for CPU isolation and frequency control)
- `numactl` package: `sudo apt-get install numactl`
- Isolated CPUs (recommended) or dedicated benchmark machine

## Additional Requirements

Install additional tools for enhanced benchmarking:

```bash
sudo apt-get install numactl cpufrequtils  # CPU and NUMA control
sudo apt-get install python3-scipy          # For confidence intervals
```

## Step 1: System Setup

### Option A: Permanent CPU Isolation (Recommended)

Edit `/etc/default/grub` and add to `GRUB_CMDLINE_LINUX`:

```bash
isolcpus=2,3 nohz_full=2,3 rcu_nocbs=2,3
```

This isolates CPUs 2 and 3 (adjust based on your system). Then:

```bash
sudo update-grub
sudo reboot
```

### Option B: Temporary Setup (Current Session)

Run the setup script:

```bash
cd benchmarking
sudo bash scripts/setup_paper_benchmarks.sh
```

This will:
- Set CPU governor to `performance`
- Disable turbo boost
- Set CPUs to maximum frequency
- Detect NUMA topology
- Save configuration to `.benchmark_config`

## Step 2: Run Benchmarks

### With Statistical Analysis

```bash
# Set number of runs (default: 5)
export BENCHMARK_RUNS=10

# Run all benchmarks with CPU isolation and NUMA pinning
bash scripts/run_paper_benchmarks.sh
```

This script:
- Runs each benchmark multiple times (default: 5)
- Uses CPU isolation (`taskset` or `numactl`)
- Pins to specific NUMA node
- Computes statistics: mean, std dev, min, max
- Outputs aggregated results with statistical data

### Output Format

The statistical output includes:
- `Time_ns`: Mean time (nanoseconds)
- `Time_std`: Standard deviation
- `Time_min`: Minimum time
- `Time_max`: Maximum time

Example:
```csv
Pattern,Engine,Input_Size,Time_ns,Time_std,Time_min,Time_max,Matches
Simple/digits,CTRE-SIMD,16,7.85,0.12,7.70,8.10,10000
```

## Step 3: Verify System State

Before running benchmarks, verify:

```bash
# Check CPU frequencies
cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_cur_freq

# Check governor
cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

# Check turbo boost status
cat /sys/devices/system/cpu/intel_pstate/no_turbo  # Intel
# or
cat /sys/devices/system/cpu/cpufreq/boost  # AMD

# Check NUMA topology
numactl --hardware
```

## Step 4: Generate Plots

Plots will automatically use mean values and can display error bars if statistical data is available:

```bash
python3 generate.py
```

## Additional Considerations

### 1. Multiple Runs

For publication, run benchmarks multiple times:
- **Minimum**: 5 runs
- **Recommended**: 10-20 runs
- **For critical results**: 30+ runs

Set via environment variable:
```bash
export BENCHMARK_RUNS=20
bash scripts/run_paper_benchmarks.sh
```

### 2. System Documentation

Record system information for the paper:

```bash
# CPU info
lscpu > system_info.txt

# Kernel version
uname -a >> system_info.txt

# Compiler version
g++ --version >> system_info.txt

# Memory info
free -h >> system_info.txt

# NUMA info
numactl --hardware >> system_info.txt
```

### 3. Warmup Periods

The benchmark code includes warmup periods (3 iterations by default). For very short benchmarks, consider increasing:

```cpp
constexpr int WARMUP = 10;  // Increase for microbenchmarks
```

### 4. Confidence Intervals

For publication, compute 95% confidence intervals:

```python
import numpy as np
from scipy import stats

mean = 7.85
std = 0.12
n = 10  # number of runs

# 95% confidence interval
ci = stats.t.interval(0.95, n-1, loc=mean, scale=std/np.sqrt(n))
```

### 5. Outlier Detection

Consider removing outliers using IQR method:

```python
Q1 = np.percentile(times, 25)
Q3 = np.percentile(times, 75)
IQR = Q3 - Q1
lower_bound = Q1 - 1.5 * IQR
upper_bound = Q3 + 1.5 * IQR
filtered = [t for t in times if lower_bound <= t <= upper_bound]
```

### 6. Statistical Significance

For comparing engines, use statistical tests:

```python
from scipy import stats

# t-test for comparing two engines
t_stat, p_value = stats.ttest_ind(engine1_times, engine2_times)
if p_value < 0.05:
    print("Statistically significant difference")
```

## Troubleshooting

### CPU Isolation Not Working

- Check kernel cmdline: `cat /proc/cmdline | grep isolcpus`
- Verify isolated CPUs are not used: `ps -eo psr,comm | grep -v "\[" | sort -n`
- Use `taskset` manually: `taskset -c 2,3 ./build/bench_simd simple`

### Frequency Scaling Still Active

- Check governor: `cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor`
- Manually set: `echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor`
- Lock frequency: `cpufreq-set -c 0 -g performance -u $(cpufreq-info -l -c 0 | awk '{print $2}')`

### NUMA Issues

- Check NUMA topology: `numactl --hardware`
- Use specific node: `numactl --membind=0 --physcpubind=2,3 <command>`
- Disable NUMA balancing: `echo 0 | sudo tee /proc/sys/kernel/numa_balancing`

## Best Practices for Papers

1. **Report methodology**: Document CPU isolation, number of runs, statistical methods
2. **Include system specs**: CPU model, frequency, memory, kernel version
3. **Show variance**: Include error bars or confidence intervals in plots
4. **Multiple systems**: Run on different hardware if possible
5. **Reproducibility**: Provide exact commands and system configuration
6. **Statistical tests**: Use appropriate tests (t-test, Mann-Whitney U) for comparisons

## Example Paper Section

```
Experimental Setup: All benchmarks were run on an Intel Xeon E5-2680 v4 
(2.4 GHz, 14 cores) with 128 GB RAM running Linux 5.15.0. CPUs 2-3 were 
isolated from the scheduler (isolcpus=2,3) and pinned using numactl. 
CPU frequency scaling was disabled (governor set to 'performance') and 
turbo boost was disabled. Each benchmark was run 10 times, and we report 
mean execution time with 95% confidence intervals. All measurements were 
taken with the system in an idle state with minimal background processes.
```

