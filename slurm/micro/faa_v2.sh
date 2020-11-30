#!/bin/sh

#SBATCH --job-name=faa_v2_micro
#SBATCH --time 01:00:00
#SBATCH --nodes=1
#SBATCH --partition=standard96:test
#SBATCH -L ansys:1

sh ./micro/run_pairs_and_bursts.sh faa_v2 10M 100
