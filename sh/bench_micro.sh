#!/bin/sh

set -euo pipefail

readonly queue=$1
readonly size=$2
readonly iters=$3

readonly csv_dir="csv/$queue/$size/throughput"
mkdir -p "$csv_dir"

./build/bench_throughput "$queue" pairs $size $iters > "$out_dir/pairs.csv"
