#!/bin/sh

rm slurm*
for queue in "$@"
do
  sbatch macro/"$queue".sh
  sbatch micro/"$queue".sh
done
