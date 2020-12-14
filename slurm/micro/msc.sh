#!/bin/sh

#SBATCH --job-name=msc_micro
#SBATCH --time 03:00:00
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --partition=standard96
#SBATCH -L ansys:1

sh ./micro/run_pairs_and_bursts.sh msc 10M 100
