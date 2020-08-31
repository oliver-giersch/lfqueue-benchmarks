#!/bin/sh

#SBATCH --job-name=faa+_macro
#SBATCH --time 00:10:00
#SBATCH --nodes=1
#SBATCH --partition=standard96:test
#SBATCH -L ansys:1

sh ./macro/run_reads_and_writes.sh "faa+" 10M
