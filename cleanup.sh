#!/bin/bash
# Remove all build/benchmark artifacts. Use --dry-run to preview.
# Note: This does NOT remove benchmark output (CSVs and figures).
cd "$(cd "$(dirname "$0")" && pwd)"

DRY_RUN=false
[[ "$1" == "--dry-run" || "$1" == "-n" ]] && DRY_RUN=true && echo "DRY RUN"

rm_dir() { [ -d "$1" ] && { $DRY_RUN && echo "rm -rf $1" || rm -rf "$1"; }; }
rm_pat() { find . -name "$1" -type f 2>/dev/null | while read f; do $DRY_RUN && echo "rm $f" || rm -f "$f"; done; }

# Build directories
rm_dir "benchmarking/build"
rm_dir "tests/build"
rm_dir "build"
rm_dir "CMakeFiles"
rm_dir ".cache"
rm_dir ".ccls-cache"
rm_dir "results"

for d in cmake-build-*/; do [ -d "$d" ] && rm_dir "${d%/}"; done 2>/dev/null

# Build artifacts
rm_pat "CMakeCache.txt"
rm_pat "cmake_install.cmake"
rm_pat "*.o"
rm_pat "*.a"
rm_pat "*.so"
rm_pat "*.pyc"
rm_pat "*.tmp"
rm_pat "*~"
rm_pat "*.swp"

# Benchmark executables
for exe in bench_simd bench_scalar bench_original bench_baseline; do
    find . -name "$exe" -type f 2>/dev/null | while read f; do $DRY_RUN && echo "rm $f" || rm -f "$f"; done
done

# Python cache
$DRY_RUN || find . -type d -name "__pycache__" -exec rm -rf {} + 2>/dev/null
$DRY_RUN || find . -type d -name "CMakeFiles" -exec rm -rf {} + 2>/dev/null

[ -f "compile_commands.json" ] && { $DRY_RUN && echo "rm compile_commands.json" || rm -f compile_commands.json; }

echo "Done."
