#!/bin/bash

PPN=3

export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH

hosts="dgx-gaia-40:$PPN"

set -x

mpirun -np $PPN -H $hosts ./testbench

set +x

