#!/bin/bash
# Cleanup script - removes all benchmark and build artifacts
# Run this when you want a clean codebase for review
#
# Usage:
#   ./cleanup.sh          - Remove all artifacts
#   ./cleanup.sh --dry-run - Preview what would be removed

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

DRY_RUN=false
if [ "$1" = "--dry-run" ] || [ "$1" = "-n" ]; then
    DRY_RUN=true
    echo "=== DRY RUN - No files will be removed ==="
else
    echo "=== CTRE-SIMD Cleanup Script ==="
fi
echo ""

remove_dir() {
    local dir="$1"
    if [ -d "$dir" ]; then
        local count=$(find "$dir" -type f 2>/dev/null | wc -l)
        if [ "$DRY_RUN" = true ]; then
            echo "  Would remove $dir/ ($count files)"
        else
            rm -rf "$dir" && echo "  Removed $dir/ ($count files)"
        fi
    fi
}

remove_files() {
    local pattern="$1"
    local description="$2"
    local count=$(find . -name "$pattern" -type f 2>/dev/null | wc -l)
    if [ "$count" -gt 0 ]; then
        if [ "$DRY_RUN" = true ]; then
            echo "  Would remove $count $description"
        else
            find . -name "$pattern" -type f -delete 2>/dev/null
            echo "  Removed $count $description"
        fi
    fi
}

# Build artifacts
echo "Removing build artifacts..."
remove_dir "plots/build"
remove_dir "build"
for d in cmake-build-*/; do 
    [ -d "$d" ] && remove_dir "${d%/}"
done 2>/dev/null

# Benchmark output
echo "Removing benchmark output..."
remove_dir "plots/output"

# CMake files
echo "Removing CMake artifacts..."
remove_files "CMakeCache.txt" "CMakeCache.txt files"
remove_files "cmake_install.cmake" "cmake_install.cmake files"
remove_files "Makefile" "Makefiles"
remove_dir "CMakeFiles"
if [ "$DRY_RUN" = false ]; then
    find . -type d -name "CMakeFiles" -exec rm -rf {} + 2>/dev/null
fi

# Compiled binaries and objects
echo "Removing compiled files..."
remove_files "*.o" "object files"
remove_files "*.a" "static libraries"
remove_files "*.so" "shared libraries"

# Benchmark executables
echo "Removing benchmark executables..."
for exe in bench_simd bench_baseline benchmark_simd benchmark_baseline; do
    if find . -name "$exe" -type f 2>/dev/null | grep -q .; then
        if [ "$DRY_RUN" = true ]; then
            echo "  Would remove $exe"
        else
            find . -name "$exe" -type f -delete 2>/dev/null
            echo "  Removed $exe"
        fi
    fi
done

# Python cache
echo "Removing Python cache..."
if [ "$DRY_RUN" = true ]; then
    count=$(find . -type d -name "__pycache__" 2>/dev/null | wc -l)
    [ "$count" -gt 0 ] && echo "  Would remove $count __pycache__ directories"
else
    find . -type d -name "__pycache__" -exec rm -rf {} + 2>/dev/null
    find . -type d -name ".pytest_cache" -exec rm -rf {} + 2>/dev/null
fi
remove_files "*.pyc" "Python bytecode files"

# Editor/IDE artifacts
echo "Removing editor artifacts..."
remove_dir ".cache"
remove_dir ".ccls-cache"
if [ -f "compile_commands.json" ]; then
    if [ "$DRY_RUN" = true ]; then
        echo "  Would remove compile_commands.json"
    else
        rm -f compile_commands.json && echo "  Removed compile_commands.json"
    fi
fi

# Temporary files
echo "Removing temporary files..."
remove_files "*.tmp" "temporary files"
remove_files "*~" "backup files"
remove_files "*.swp" "vim swap files"

echo ""
if [ "$DRY_RUN" = true ]; then
    echo "=== Dry Run Complete (no files removed) ==="
else
    echo "=== Cleanup Complete ==="
fi
echo ""
echo "Preserved:"
echo "  - Source code (include/ctre/)"
echo "  - Benchmark scripts (plots/scripts/, run_individual_benchmarks.sh)"
echo "  - Benchmark source (plots/benchmarks/)"
echo "  - Plot generation (plots/generate.py, plots/plotting/)"
echo "  - Test source files (tests/*.cpp)"
echo "  - README.md"
echo ""
echo "To regenerate benchmarks: ./plots/scripts/generate_all.sh"
echo "To quick-test SIMD:       ./run_individual_benchmarks.sh"
