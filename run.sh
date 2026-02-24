#!/usr/bin/env bash
clear
rm -f *.log

set -e

# Build
echo "🛠️ Building..."
make clean && make -j$(nproc 2>/dev/null || echo 4)
cd indices/LIT && make clean && make pureLIT_3col && cd ../..
echo "✅ Build finished"

# Config
: > benchmark.log
RUNS=5

# Data/Queries
STATIC_DATA="data/Static/BTC.txt"
STATIC_QUERIES="data/Static/BTC_queries.txt"
DYNAMIC_DATA="data/Dynamic/BTC_stream.txt"

# Check if data files exist, provide unzip instructions if not found
if [[ ! -f "$STATIC_DATA" ]]; then
    echo "❌ $STATIC_DATA not found"
    echo "💡 Please unzip the data files first. Look for compressed archives in the data directory and extract them."
    exit 1
fi

if [[ ! -f "$STATIC_QUERIES" ]]; then
    echo "❌ $STATIC_QUERIES not found"
    echo "💡 Please unzip the data files first. Look for compressed archives in the data directory and extract them."
    exit 1
fi

if [[ ! -f "$DYNAMIC_DATA" ]]; then
    echo "❌ $DYNAMIC_DATA not found"
    echo "💡 Please unzip the data files first. Look for compressed archives in the data directory and extract them."
    exit 1
fi

# Static Range Queries
echo "🔬 Static Range Queries..."
./query_intervaltree.exec -r $RUNS "$STATIC_DATA" "$STATIC_QUERIES" | tee -a benchmark.log
./query_hint_m.exec -r $RUNS -m 0 "$STATIC_DATA" "$STATIC_QUERIES" | tee -a benchmark.log
./query_it_firas.exec -r $RUNS "$STATIC_DATA" "$STATIC_QUERIES" | tee -a benchmark.log
./query_ait.exec -r $RUNS "$STATIC_DATA" "$STATIC_QUERIES" | tee -a benchmark.log

echo "Static Sampling Queries..."
./query_intervaltree.exec -s 100 -r $RUNS "$STATIC_DATA" "$STATIC_QUERIES" | tee -a benchmark.log
./query_hint_m.exec -m 0 -s 100 -r $RUNS "$STATIC_DATA" "$STATIC_QUERIES" | tee -a benchmark.log
./query_it_firas.exec -s 100 -r $RUNS "$STATIC_DATA" "$STATIC_QUERIES" | tee -a benchmark.log
./query_ait.exec -s 100 -r $RUNS "$STATIC_DATA" "$STATIC_QUERIES" | tee -a benchmark.log

# Dynamic Range Queries
echo "Dynamic Range Queries..."
./indices/LIT/query_pureLIT_3col.exec -e 687640 -b ENHANCEDHASHMAP -c 10000 -r $RUNS "$DYNAMIC_DATA" | tee -a benchmark.log
./query_stream_ait.exec -r $RUNS "$DYNAMIC_DATA" | tee -a benchmark.log
./query_stream_firas_dit.exec -r $RUNS "$DYNAMIC_DATA" | tee -a benchmark.log

echo "Dynamic Sampling Queries..."
./indices/LIT/query_pureLIT_3col.exec -s 100 -e 687640 -b ENHANCEDHASHMAP -c 10000 -r $RUNS "$DYNAMIC_DATA" | tee -a benchmark.log
./query_stream_ait.exec -s 100 -r $RUNS "$DYNAMIC_DATA" | tee -a benchmark.log
./query_stream_firas_dit.exec -s 100 -r $RUNS "$DYNAMIC_DATA" | tee -a benchmark.log

echo "All experiments finished. Complete output in benchmark.log"
