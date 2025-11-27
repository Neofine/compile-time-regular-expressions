# Benchmarks

## Primary Benchmark

**`run_master_benchmark.sh`** - Main performance benchmark
- Compares SIMD vs non-SIMD performance
- Tests 36 patterns across various sizes
- Clean output with overall statistics
- **Usage:** `./run_master_benchmark.sh`

## Testing

**`run_all_tests.sh`** - Unit and integration tests
- Runs all test suites
- **Usage:** `./run_all_tests.sh`

## Results

Current performance: **1.7-2.0x average speedup** with SIMD optimizations

Best performers:
- Large character classes (`[a-z]*_128`): **10x+**
- Medium ranges (`[a-z]+_64`): **6-7x**
- Digit patterns (`[0-9]*`): **5-6x**
