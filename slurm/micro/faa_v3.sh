#!/bin/sh

#SBATCH --job-name=faa_v3_micro
#SBATCH --time 01:00:00
#SBATCH --nodes=1
#SBATCH --partition=standard96:test
#SBATCH -L ansys:1

sh ./micro/run_pairs_and_bursts.sh faa_v3 10M 100
