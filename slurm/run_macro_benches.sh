#!/bin/sh

rm slurm*

sbatch macro/faa.sh
sbatch macro/faa_plus.sh
sbatch macro/lcr.sh
sbatch macro/loo.sh
#if [ "$0" == "--all" ]; then
#fi

