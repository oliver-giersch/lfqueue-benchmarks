#!/bin/sh

#SBATCH --job-name=ymc_macro
#SBATCH --time 01:00:00
#SBATCH --nodes=1
#SBATCH --partition=standard96:test
#SBATCH -L ansys:1

sh ./macro/run_reads_and_writes.sh ymc 10M 100
