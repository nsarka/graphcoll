#!/bin/bash

export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH

hosts="dgx-gaia-26:8"

set -x

mpirun -np 8 -H $hosts ./testbench

set +x

