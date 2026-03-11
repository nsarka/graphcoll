#!/bin/bash

PPN=3

export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH

hosts="dgx-gaia-58:$PPN"

set -x

mpirun --tag-output -np $PPN -H $hosts ./testbench

set +x

