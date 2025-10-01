#!/bin/bash

export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH

hosts="dgx-gaia-46:2"

set -x

mpirun -np 2 -H $hosts ./testbench

set +x

