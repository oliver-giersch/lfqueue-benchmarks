#!/bin/sh

rm slurm*

sbatch micro/faa.sh
sbatch micro/faa_plus.sh
sbatch micro/lcr.sh
sbatch micro/loo.sh
sbatch micro/ymc.sh
