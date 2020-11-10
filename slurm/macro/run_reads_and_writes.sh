#!/bin/sh

queue=$1
size=$2
iters=$3

parent_dir=$HOME/projects/looqueue-benchmarks
out_dir=$parent_dir/csv/$queue/$size/throughput

mkdir -p $out_dir
cd $parent_dir/cmake-build-remote-release || exit
./bench_throughput $queue reads  $size $iters > $out_dir/reads.csv
./bench_throughput $queue writes $size $iters > $out_dir/writes.csv
./bench_throughput $queue mixed $size $iters > $out_dir/mixed.csv
