#!/bin/sh

#SBATCH --job-name=lcr_macro
#SBATCH --time 00:30:00
#SBATCH --nodes=1
#SBATCH --partition=standard96:test
#SBATCH -L ansys:1

sh ./macro/run_reads_and_writes.sh lcr 50M
