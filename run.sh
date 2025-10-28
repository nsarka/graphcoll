#!/bin/bash

PPN=2

export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH

hosts="dgx-gaia-26:$PPN"

set -x

mpirun -np $PPN -H $hosts ./testbench

set +x

