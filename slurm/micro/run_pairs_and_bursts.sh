#!/bin/sh

queue=$1
size=$2
iters=$3

parent_dir=$HOME/projects/looqueue-benchmarks
out_dir=$parent_dir/csv/$queue/$size/throughput

mkdir -p $out_dir
cd $parent_dir/cmake-build-remote-release || exit
./bench_throughput $queue pairs  $size $iters > $out_dir/pairs.csv
./bench_throughput $queue bursts $size $iters > $out_dir/bursts.csv
