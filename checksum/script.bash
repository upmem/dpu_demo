#!/bin/bash

set -e

run_rank() {
    while true
    do
        ./build/checksum_host
    done
}

for ((c=0; c < NB_RANK - 1; c++))
do
    run_rank $c &
done

run_rank $(($NB_RANK - 1))
