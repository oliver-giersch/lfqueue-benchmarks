#!/bin/sh

#SBATCH --job-name=msc_macro
#SBATCH --time 10:00:00
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --partition=standard96
#SBATCH -L ansys:1

sh ./macro/run_reads_and_writes.sh msc 10M 25
