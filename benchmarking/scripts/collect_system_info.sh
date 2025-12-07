#!/bin/bash
# Collect system information for paper documentation

OUTPUT_FILE="${1:-system_info.txt}"

echo "Collecting system information..."
echo "==========================================" > "$OUTPUT_FILE"
echo "System Information for Benchmark Paper" >> "$OUTPUT_FILE"
echo "Generated: $(date)" >> "$OUTPUT_FILE"
echo "==========================================" >> "$OUTPUT_FILE"
echo "" >> "$OUTPUT_FILE"

echo "CPU Information:" >> "$OUTPUT_FILE"
echo "---------------" >> "$OUTPUT_FILE"
lscpu >> "$OUTPUT_FILE"
echo "" >> "$OUTPUT_FILE"

echo "Kernel Information:" >> "$OUTPUT_FILE"
echo "-------------------" >> "$OUTPUT_FILE"
uname -a >> "$OUTPUT_FILE"
echo "" >> "$OUTPUT_FILE"

echo "Compiler Information:" >> "$OUTPUT_FILE"
echo "--------------------" >> "$OUTPUT_FILE"
g++ --version >> "$OUTPUT_FILE" 2>&1 || echo "g++ not found" >> "$OUTPUT_FILE"
clang++ --version >> "$OUTPUT_FILE" 2>&1 || echo "clang++ not found" >> "$OUTPUT_FILE"
echo "" >> "$OUTPUT_FILE"

echo "Memory Information:" >> "$OUTPUT_FILE"
echo "------------------" >> "$OUTPUT_FILE"
free -h >> "$OUTPUT_FILE"
echo "" >> "$OUTPUT_FILE"

echo "NUMA Topology:" >> "$OUTPUT_FILE"
echo "-------------" >> "$OUTPUT_FILE"
if command -v numactl &> /dev/null; then
    numactl --hardware >> "$OUTPUT_FILE"
else
    echo "numactl not available" >> "$OUTPUT_FILE"
fi
echo "" >> "$OUTPUT_FILE"

echo "CPU Frequency Settings:" >> "$OUTPUT_FILE"
echo "---------------------" >> "$OUTPUT_FILE"
for cpu in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do
    if [ -f "$cpu" ]; then
        CPU_NUM=$(dirname "$cpu" | xargs basename | sed 's/cpu//')
        GOV=$(cat "$cpu")
        CUR_FREQ=$(cat "/sys/devices/system/cpu/cpu${CPU_NUM}/cpufreq/scaling_cur_freq" 2>/dev/null || echo "N/A")
        MAX_FREQ=$(cat "/sys/devices/system/cpu/cpu${CPU_NUM}/cpufreq/scaling_max_freq" 2>/dev/null || echo "N/A")
        MIN_FREQ=$(cat "/sys/devices/system/cpu/cpu${CPU_NUM}/cpufreq/scaling_min_freq" 2>/dev/null || echo "N/A")
        
        if [ "$CUR_FREQ" != "N/A" ]; then
            CUR_MHZ=$((CUR_FREQ / 1000))
            MAX_MHZ=$((MAX_FREQ / 1000))
            MIN_MHZ=$((MIN_FREQ / 1000))
            echo "CPU $CPU_NUM: governor=$GOV, freq=${CUR_MHZ}MHz (min=${MIN_MHZ}MHz, max=${MAX_MHZ}MHz)" >> "$OUTPUT_FILE"
        else
            echo "CPU $CPU_NUM: governor=$GOV" >> "$OUTPUT_FILE"
        fi
    fi
done
echo "" >> "$OUTPUT_FILE"

echo "Turbo Boost Status:" >> "$OUTPUT_FILE"
echo "------------------" >> "$OUTPUT_FILE"
if [ -f /sys/devices/system/cpu/intel_pstate/no_turbo ]; then
    TURBO=$(cat /sys/devices/system/cpu/intel_pstate/no_turbo)
    if [ "$TURBO" = "1" ]; then
        echo "Intel Turbo Boost: DISABLED" >> "$OUTPUT_FILE"
    else
        echo "Intel Turbo Boost: ENABLED" >> "$OUTPUT_FILE"
    fi
elif [ -f /sys/devices/system/cpu/cpufreq/boost ]; then
    BOOST=$(cat /sys/devices/system/cpu/cpufreq/boost)
    if [ "$BOOST" = "0" ]; then
        echo "CPU Boost: DISABLED" >> "$OUTPUT_FILE"
    else
        echo "CPU Boost: ENABLED" >> "$OUTPUT_FILE"
    fi
else
    echo "Turbo boost control not available" >> "$OUTPUT_FILE"
fi
echo "" >> "$OUTPUT_FILE"

echo "Isolated CPUs:" >> "$OUTPUT_FILE"
echo "-------------" >> "$OUTPUT_FILE"
ISOLATED=$(cat /proc/cmdline | grep -o 'isolcpus=[0-9,-]*' || echo "None")
echo "$ISOLATED" >> "$OUTPUT_FILE"
echo "" >> "$OUTPUT_FILE"

echo "Library Versions:" >> "$OUTPUT_FILE"
echo "----------------" >> "$OUTPUT_FILE"
if pkg-config --exists re2; then
    echo "RE2: $(pkg-config --modversion re2)" >> "$OUTPUT_FILE"
fi
if pkg-config --exists libpcre2-8; then
    echo "PCRE2: $(pkg-config --modversion libpcre2-8)" >> "$OUTPUT_FILE"
fi
if pkg-config --exists libhs; then
    echo "Hyperscan: $(pkg-config --modversion libhs)" >> "$OUTPUT_FILE"
fi
echo "" >> "$OUTPUT_FILE"

echo "System Information saved to: $OUTPUT_FILE"
cat "$OUTPUT_FILE"

