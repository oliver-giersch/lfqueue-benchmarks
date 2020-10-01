#!/bin/sh

queue=$1
size=$2

parent_dir=$HOME/projects/looqueue-benchmarks
out_dir=$parent_dir/csv/$queue/$size/throughput

mkdir -p $out_dir
cd $parent_dir/cmake-build-remote-release || exit
./bench_throughput $queue pairs  $size 20 > $out_dir/pairs.csv
./bench_throughput $queue bursts $size 20 > $out_dir/bursts.csv
