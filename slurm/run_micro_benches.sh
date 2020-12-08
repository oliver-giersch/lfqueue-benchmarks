#!/bin/sh

rm slurm*

sbatch micro/faa.sh
sbatch micro/faa_v1.sh
sbatch micro/faa_v2.sh
sbatch micro/faa_v3.sh
sbatch micro/lcr.sh
sbatch micro/loo.sh
sbatch micro/lsc.sh
sbatch micro/ymc.sh
