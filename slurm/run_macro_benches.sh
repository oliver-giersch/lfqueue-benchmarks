#!/bin/sh

rm slurm*

sbatch macro/faa.sh
sbatch macro/faa_v1.sh
sbatch macro/faa_v2.sh
sbatch macro/faa_v3.sh
sbatch macro/lcr.sh
sbatch macro/loo.sh
sbatch macro/scq2.sh
sbatch macro/scqd.sh
sbatch macro/ymc.sh

